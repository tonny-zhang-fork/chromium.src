// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/scheduler/renderer_scheduler_impl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/output/begin_frame_args.h"
#include "content/renderer/scheduler/nestable_single_thread_task_runner.h"
#include "content/renderer/scheduler/renderer_task_queue_selector.h"
#include "ui/gfx/frame_time.h"

namespace content {

RendererSchedulerImpl::RendererSchedulerImpl(
    scoped_refptr<NestableSingleThreadTaskRunner> main_task_runner)
    : renderer_task_queue_selector_(new RendererTaskQueueSelector()),
      task_queue_manager_(
          new TaskQueueManager(TASK_QUEUE_COUNT,
                               main_task_runner,
                               renderer_task_queue_selector_.get())),
      control_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(CONTROL_TASK_QUEUE)),
      control_task_after_wakeup_runner_(task_queue_manager_->TaskRunnerForQueue(
          CONTROL_TASK_AFTER_WAKEUP_QUEUE)),
      default_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(DEFAULT_TASK_QUEUE)),
      compositor_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(COMPOSITOR_TASK_QUEUE)),
      loading_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(LOADING_TASK_QUEUE)),
      delayed_update_policy_runner_(
          base::Bind(&RendererSchedulerImpl::UpdatePolicy,
                     base::Unretained(this)),
          control_task_runner_),
      current_policy_(Policy::NORMAL),
      idle_period_state_(IdlePeriodState::NOT_IN_IDLE_PERIOD),
      long_idle_periods_enabled_(false),
      last_input_type_(blink::WebInputEvent::Undefined),
      input_stream_state_(InputStreamState::INACTIVE),
      policy_may_need_update_(&incoming_signals_lock_),
      weak_factory_(this) {
  weak_renderer_scheduler_ptr_ = weak_factory_.GetWeakPtr();
  update_policy_closure_ = base::Bind(&RendererSchedulerImpl::UpdatePolicy,
                                      weak_renderer_scheduler_ptr_);
  end_idle_period_closure_.Reset(base::Bind(
      &RendererSchedulerImpl::EndIdlePeriod, weak_renderer_scheduler_ptr_));
  initiate_next_long_idle_period_closure_.Reset(base::Bind(
      &RendererSchedulerImpl::InitiateLongIdlePeriod,
      weak_renderer_scheduler_ptr_));
  initiate_next_long_idle_period_after_wakeup_closure_.Reset(base::Bind(
      &RendererSchedulerImpl::InitiateLongIdlePeriodAfterWakeup,
      weak_renderer_scheduler_ptr_));

  idle_task_runner_ = make_scoped_refptr(new SingleThreadIdleTaskRunner(
      task_queue_manager_->TaskRunnerForQueue(IDLE_TASK_QUEUE),
      control_task_after_wakeup_runner_,
      base::Bind(&RendererSchedulerImpl::CurrentIdleTaskDeadlineCallback,
                 weak_renderer_scheduler_ptr_)));

  renderer_task_queue_selector_->SetQueuePriority(
      CONTROL_TASK_QUEUE, RendererTaskQueueSelector::CONTROL_PRIORITY);

  renderer_task_queue_selector_->SetQueuePriority(
      CONTROL_TASK_AFTER_WAKEUP_QUEUE,
      RendererTaskQueueSelector::CONTROL_PRIORITY);
  task_queue_manager_->SetPumpPolicy(
      CONTROL_TASK_AFTER_WAKEUP_QUEUE,
      TaskQueueManager::PumpPolicy::AFTER_WAKEUP);

  renderer_task_queue_selector_->DisableQueue(IDLE_TASK_QUEUE);
  task_queue_manager_->SetPumpPolicy(IDLE_TASK_QUEUE,
                                     TaskQueueManager::PumpPolicy::MANUAL);

  // TODO(skyostil): Increase this to 4 (crbug.com/444764).
  task_queue_manager_->SetWorkBatchSize(1);

  for (size_t i = 0; i < TASK_QUEUE_COUNT; i++) {
    task_queue_manager_->SetQueueName(
        i, TaskQueueIdToString(static_cast<QueueId>(i)));
  }
  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
}

RendererSchedulerImpl::~RendererSchedulerImpl() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
}

void RendererSchedulerImpl::Shutdown() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  task_queue_manager_.reset();
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::DefaultTaskRunner() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return default_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::CompositorTaskRunner() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return compositor_task_runner_;
}

scoped_refptr<SingleThreadIdleTaskRunner>
RendererSchedulerImpl::IdleTaskRunner() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return idle_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::LoadingTaskRunner() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return loading_task_runner_;
}

void RendererSchedulerImpl::WillBeginFrame(const cc::BeginFrameArgs& args) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::WillBeginFrame", "args", args.AsValue());
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (!task_queue_manager_)
    return;

  EndIdlePeriod();
  estimated_next_frame_begin_ = args.frame_time + args.interval;
  // TODO(skyostil): Wire up real notification of input events processing
  // instead of this approximation.
  DidProcessInputEvent(args.frame_time);
}

void RendererSchedulerImpl::DidCommitFrameToCompositor() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidCommitFrameToCompositor");
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (!task_queue_manager_)
    return;

  base::TimeTicks now(Now());
  if (now < estimated_next_frame_begin_) {
    // TODO(rmcilroy): Consider reducing the idle period based on the runtime of
    // the next pending delayed tasks (as currently done in for long idle times)
    StartIdlePeriod(IdlePeriodState::IN_SHORT_IDLE_PERIOD);
    control_task_runner_->PostDelayedTask(FROM_HERE,
                                          end_idle_period_closure_.callback(),
                                          estimated_next_frame_begin_ - now);
  }
}

void RendererSchedulerImpl::BeginFrameNotExpectedSoon() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::BeginFrameNotExpectedSoon");
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (!task_queue_manager_)
    return;

  // TODO(skyostil): Wire up real notification of input events processing
  // instead of this approximation.
  DidProcessInputEvent(base::TimeTicks());

  InitiateLongIdlePeriod();
}

void RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread(
    const blink::WebInputEvent& web_input_event) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread");
  // We regard MouseMove events with the left mouse button down as a signal
  // that the user is doing something requiring a smooth frame rate.
  if (web_input_event.type == blink::WebInputEvent::MouseMove &&
      (web_input_event.modifiers & blink::WebInputEvent::LeftButtonDown)) {
    UpdateForInputEvent(web_input_event.type);
    return;
  }
  // Ignore all other mouse events because they probably don't signal user
  // interaction needing a smooth framerate. NOTE isMouseEventType returns false
  // for mouse wheel events, hence we regard them as user input.
  // Ignore keyboard events because it doesn't really make sense to enter
  // compositor priority for them.
  if (blink::WebInputEvent::isMouseEventType(web_input_event.type) ||
      blink::WebInputEvent::isKeyboardEventType(web_input_event.type)) {
    return;
  }
  UpdateForInputEvent(web_input_event.type);
}

void RendererSchedulerImpl::DidAnimateForInputOnCompositorThread() {
  UpdateForInputEvent(blink::WebInputEvent::Undefined);
}

void RendererSchedulerImpl::UpdateForInputEvent(
    blink::WebInputEvent::Type type) {
  base::AutoLock lock(incoming_signals_lock_);

  InputStreamState new_input_stream_state =
      ComputeNewInputStreamState(input_stream_state_, type, last_input_type_);

  if (input_stream_state_ != new_input_stream_state) {
    // Update scheduler policy if we should start a new policy mode.
    input_stream_state_ = new_input_stream_state;
    EnsureUrgentPolicyUpdatePostedOnMainThread(FROM_HERE);
  }
  last_input_receipt_time_on_compositor_ = Now();
  // Clear the last known input processing time so that we know an input event
  // is still queued up. This timestamp will be updated the next time the
  // compositor commits or becomes quiescent. Note that this function is always
  // called before the input event is processed either on the compositor or
  // main threads.
  last_input_process_time_on_main_ = base::TimeTicks();
  last_input_type_ = type;
}

void RendererSchedulerImpl::DidProcessInputEvent(
    base::TimeTicks begin_frame_time) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  base::AutoLock lock(incoming_signals_lock_);
  if (input_stream_state_ == InputStreamState::INACTIVE)
    return;
  // Avoid marking input that arrived after the BeginFrame as processed.
  if (!begin_frame_time.is_null() &&
      begin_frame_time < last_input_receipt_time_on_compositor_)
    return;
  last_input_process_time_on_main_ = Now();
  UpdatePolicyLocked();
}

bool RendererSchedulerImpl::IsHighPriorityWorkAnticipated() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (!task_queue_manager_)
    return false;

  MaybeUpdatePolicy();
  // The touchstart and compositor policies indicate a strong likelihood of
  // high-priority work in the near future.
  return SchedulerPolicy() == Policy::COMPOSITOR_PRIORITY ||
         SchedulerPolicy() == Policy::TOUCHSTART_PRIORITY;
}

bool RendererSchedulerImpl::ShouldYieldForHighPriorityWork() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (!task_queue_manager_)
    return false;

  MaybeUpdatePolicy();
  // We only yield if we are in the compositor priority and there is compositor
  // work outstanding, or if we are in the touchstart response priority.
  // Note: even though the control queue is higher priority we don't yield for
  // it since these tasks are not user-provided work and they are only intended
  // to run before the next task, not interrupt the tasks.
  switch (SchedulerPolicy()) {
    case Policy::NORMAL:
      return false;

    case Policy::COMPOSITOR_PRIORITY:
      return !task_queue_manager_->IsQueueEmpty(COMPOSITOR_TASK_QUEUE);

    case Policy::TOUCHSTART_PRIORITY:
      return true;

    default:
      NOTREACHED();
      return false;
  }
}

void RendererSchedulerImpl::CurrentIdleTaskDeadlineCallback(
    base::TimeTicks* deadline_out) const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  *deadline_out = estimated_next_frame_begin_;
}

RendererSchedulerImpl::Policy RendererSchedulerImpl::SchedulerPolicy() const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return current_policy_;
}

void RendererSchedulerImpl::MaybeUpdatePolicy() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (policy_may_need_update_.IsSet()) {
    UpdatePolicy();
  }
}

void RendererSchedulerImpl::EnsureUrgentPolicyUpdatePostedOnMainThread(
    const tracked_objects::Location& from_here) {
  // TODO(scheduler-dev): Check that this method isn't called from the main
  // thread.
  incoming_signals_lock_.AssertAcquired();
  if (!policy_may_need_update_.IsSet()) {
    policy_may_need_update_.SetWhileLocked(true);
    control_task_runner_->PostTask(from_here, update_policy_closure_);
  }
}

void RendererSchedulerImpl::UpdatePolicy() {
  base::AutoLock lock(incoming_signals_lock_);
  UpdatePolicyLocked();
}

void RendererSchedulerImpl::UpdatePolicyLocked() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  incoming_signals_lock_.AssertAcquired();
  if (!task_queue_manager_)
    return;

  base::TimeTicks now = Now();
  policy_may_need_update_.SetWhileLocked(false);

  base::TimeDelta new_policy_duration;
  Policy new_policy = ComputeNewPolicy(now, &new_policy_duration);
  if (new_policy_duration > base::TimeDelta()) {
    current_policy_expiration_time_ = now + new_policy_duration;
    delayed_update_policy_runner_.SetDeadline(FROM_HERE, new_policy_duration,
                                              now);
  } else {
    current_policy_expiration_time_ = base::TimeTicks();
  }

  if (new_policy == current_policy_)
    return;

  switch (new_policy) {
    case Policy::COMPOSITOR_PRIORITY:
      renderer_task_queue_selector_->SetQueuePriority(
          COMPOSITOR_TASK_QUEUE, RendererTaskQueueSelector::HIGH_PRIORITY);
      // TODO(scheduler-dev): Add a task priority between HIGH and BEST_EFFORT
      // that still has some guarantee of running.
      renderer_task_queue_selector_->SetQueuePriority(
          LOADING_TASK_QUEUE, RendererTaskQueueSelector::BEST_EFFORT_PRIORITY);
      break;
    case Policy::TOUCHSTART_PRIORITY:
      renderer_task_queue_selector_->SetQueuePriority(
          COMPOSITOR_TASK_QUEUE, RendererTaskQueueSelector::HIGH_PRIORITY);
      renderer_task_queue_selector_->DisableQueue(LOADING_TASK_QUEUE);
      break;
    case Policy::NORMAL:
      renderer_task_queue_selector_->SetQueuePriority(
          COMPOSITOR_TASK_QUEUE, RendererTaskQueueSelector::NORMAL_PRIORITY);
      renderer_task_queue_selector_->SetQueuePriority(
          LOADING_TASK_QUEUE, RendererTaskQueueSelector::NORMAL_PRIORITY);
      break;
  }
  DCHECK(renderer_task_queue_selector_->IsQueueEnabled(COMPOSITOR_TASK_QUEUE));
  if (new_policy != Policy::TOUCHSTART_PRIORITY)
    DCHECK(renderer_task_queue_selector_->IsQueueEnabled(LOADING_TASK_QUEUE));

  current_policy_ = new_policy;

  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this, AsValueLocked(now));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "RendererScheduler.policy", current_policy_);
}

RendererSchedulerImpl::Policy RendererSchedulerImpl::ComputeNewPolicy(
    base::TimeTicks now,
    base::TimeDelta* new_policy_duration) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  incoming_signals_lock_.AssertAcquired();

  Policy new_policy = Policy::NORMAL;
  *new_policy_duration = base::TimeDelta();

  if (input_stream_state_ == InputStreamState::INACTIVE)
    return new_policy;

  Policy input_priority_policy =
      input_stream_state_ ==
              InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE
          ? Policy::TOUCHSTART_PRIORITY
          : Policy::COMPOSITOR_PRIORITY;
  base::TimeDelta time_left_in_policy = TimeLeftInInputEscalatedPolicy(now);
  if (time_left_in_policy > base::TimeDelta()) {
    new_policy = input_priority_policy;
    *new_policy_duration = time_left_in_policy;
  } else {
    // Reset |input_stream_state_| to ensure
    // DidReceiveInputEventOnCompositorThread will post an UpdatePolicy task
    // when it's next called.
    input_stream_state_ = InputStreamState::INACTIVE;
  }
  return new_policy;
}

base::TimeDelta RendererSchedulerImpl::TimeLeftInInputEscalatedPolicy(
    base::TimeTicks now) const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  // TODO(rmcilroy): Change this to DCHECK_EQ when crbug.com/463869 is fixed.
  DCHECK(input_stream_state_ != InputStreamState::INACTIVE);
  incoming_signals_lock_.AssertAcquired();

  base::TimeDelta escalated_priority_duration =
      base::TimeDelta::FromMilliseconds(kPriorityEscalationAfterInputMillis);
  base::TimeDelta time_left_in_policy;
  if (last_input_process_time_on_main_.is_null() &&
      !task_queue_manager_->IsQueueEmpty(COMPOSITOR_TASK_QUEUE)) {
    // If the input event is still pending, go into input prioritized policy
    // and check again later.
    time_left_in_policy = escalated_priority_duration;
  } else {
    // Otherwise make sure the input prioritization policy ends on time.
    base::TimeTicks new_priority_end(
        std::max(last_input_receipt_time_on_compositor_,
                 last_input_process_time_on_main_) +
        escalated_priority_duration);
    time_left_in_policy = new_priority_end - now;
  }
  return time_left_in_policy;
}

RendererSchedulerImpl::IdlePeriodState
RendererSchedulerImpl::ComputeNewLongIdlePeriodState(
    const base::TimeTicks now,
    base::TimeDelta* next_long_idle_period_delay_out) {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  MaybeUpdatePolicy();
  if (SchedulerPolicy() == Policy::TOUCHSTART_PRIORITY) {
    // Don't start a long idle task in touch start priority, try again when
    // the policy is scheduled to end.
    *next_long_idle_period_delay_out = current_policy_expiration_time_ - now;
    return IdlePeriodState::NOT_IN_IDLE_PERIOD;
  }

  base::TimeTicks next_pending_delayed_task =
      task_queue_manager_->NextPendingDelayedTaskRunTime();
  base::TimeDelta max_long_idle_period_duration =
      base::TimeDelta::FromMilliseconds(kMaximumIdlePeriodMillis);
  base::TimeDelta long_idle_period_duration;
  if (next_pending_delayed_task.is_null()) {
    long_idle_period_duration = max_long_idle_period_duration;
  } else {
    // Limit the idle period duration to be before the next pending task.
    long_idle_period_duration = std::min(next_pending_delayed_task - now,
                                         max_long_idle_period_duration);
  }

  if (long_idle_period_duration > base::TimeDelta()) {
    *next_long_idle_period_delay_out = long_idle_period_duration;
    return long_idle_period_duration == max_long_idle_period_duration
               ? IdlePeriodState::IN_LONG_IDLE_PERIOD_WITH_MAX_DEADLINE
               : IdlePeriodState::IN_LONG_IDLE_PERIOD;
  } else {
    // If we can't start the idle period yet then try again after wakeup.
    *next_long_idle_period_delay_out = base::TimeDelta::FromMilliseconds(
        kRetryInitiateLongIdlePeriodDelayMillis);
    return IdlePeriodState::NOT_IN_IDLE_PERIOD;
  }
}

void RendererSchedulerImpl::InitiateLongIdlePeriod() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "InitiateLongIdlePeriod");
  DCHECK(main_thread_checker_.CalledOnValidThread());

  // End any previous idle period.
  EndIdlePeriod();

  base::TimeTicks now(Now());
  base::TimeDelta next_long_idle_period_delay;
  IdlePeriodState new_idle_period_state =
      ComputeNewLongIdlePeriodState(now, &next_long_idle_period_delay);
  if (long_idle_periods_enabled_ && IsInIdlePeriod(new_idle_period_state)) {
    estimated_next_frame_begin_ = now + next_long_idle_period_delay;
    StartIdlePeriod(new_idle_period_state);
  }

  if (task_queue_manager_->IsQueueEmpty(IDLE_TASK_QUEUE)) {
    // If there are no current idle tasks then post the call to initiate the
    // next idle for execution after wakeup (at which point after-wakeup idle
    // tasks might be eligible to run or more idle tasks posted).
    control_task_after_wakeup_runner_->PostDelayedTask(
        FROM_HERE,
        initiate_next_long_idle_period_after_wakeup_closure_.callback(),
        next_long_idle_period_delay);
  } else {
    // Otherwise post on the normal control task queue.
    control_task_runner_->PostDelayedTask(
        FROM_HERE,
        initiate_next_long_idle_period_closure_.callback(),
        next_long_idle_period_delay);
  }
}

void RendererSchedulerImpl::InitiateLongIdlePeriodAfterWakeup() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "InitiateLongIdlePeriodAfterWakeup");
  DCHECK(main_thread_checker_.CalledOnValidThread());

  if (IsInIdlePeriod(idle_period_state_)) {
    // Since we were asleep until now, end the async idle period trace event at
    // the time when it would have ended were we awake.
    TRACE_EVENT_ASYNC_END_WITH_TIMESTAMP0(
        "renderer.scheduler", "RendererSchedulerIdlePeriod", this,
        std::min(estimated_next_frame_begin_, Now()).ToInternalValue());
    idle_period_state_ = IdlePeriodState::ENDING_LONG_IDLE_PERIOD;
    EndIdlePeriod();
  }

  // Post a task to initiate the next long idle period rather than calling it
  // directly to allow all pending PostIdleTaskAfterWakeup tasks to get enqueued
  // on the idle task queue before the next idle period starts so they are
  // eligible to be run during the new idle period.
  control_task_runner_->PostTask(
      FROM_HERE,
      initiate_next_long_idle_period_closure_.callback());
}

void RendererSchedulerImpl::StartIdlePeriod(IdlePeriodState new_state) {
  TRACE_EVENT_ASYNC_BEGIN0("renderer.scheduler",
                           "RendererSchedulerIdlePeriod", this);
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK(IsInIdlePeriod(new_state));

  renderer_task_queue_selector_->EnableQueue(
      IDLE_TASK_QUEUE, RendererTaskQueueSelector::BEST_EFFORT_PRIORITY);
  task_queue_manager_->PumpQueue(IDLE_TASK_QUEUE);
  idle_period_state_ = new_state;
}

void RendererSchedulerImpl::EndIdlePeriod() {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  end_idle_period_closure_.Cancel();
  initiate_next_long_idle_period_closure_.Cancel();
  initiate_next_long_idle_period_after_wakeup_closure_.Cancel();

  // If we weren't already within an idle period then early-out.
  if (!IsInIdlePeriod(idle_period_state_))
    return;

  // If we are in the ENDING_LONG_IDLE_PERIOD state we have already logged the
  // trace event.
  if (idle_period_state_ != IdlePeriodState::ENDING_LONG_IDLE_PERIOD) {
    bool is_tracing;
    TRACE_EVENT_CATEGORY_GROUP_ENABLED("renderer.scheduler", &is_tracing);
    if (is_tracing && !estimated_next_frame_begin_.is_null() &&
        base::TimeTicks::Now() > estimated_next_frame_begin_) {
      TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(
          "renderer.scheduler",
          "RendererSchedulerIdlePeriod",
          this,
          "DeadlineOverrun",
          estimated_next_frame_begin_.ToInternalValue());
    }
    TRACE_EVENT_ASYNC_END0("renderer.scheduler",
                           "RendererSchedulerIdlePeriod", this);
  }

  renderer_task_queue_selector_->DisableQueue(IDLE_TASK_QUEUE);
  idle_period_state_ = IdlePeriodState::NOT_IN_IDLE_PERIOD;
}

// static
bool RendererSchedulerImpl::IsInIdlePeriod(IdlePeriodState state) {
  return state != IdlePeriodState::NOT_IN_IDLE_PERIOD;
}

bool RendererSchedulerImpl::CanExceedIdleDeadlineIfRequired() const {
  TRACE_EVENT_BEGIN0("renderer.scheduler", "CanExceedIdleDeadlineIfRequired");
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return idle_period_state_ ==
         IdlePeriodState::IN_LONG_IDLE_PERIOD_WITH_MAX_DEADLINE;
}

void RendererSchedulerImpl::SetTimeSourceForTesting(
    scoped_refptr<cc::TestNowSource> time_source) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  time_source_ = time_source;
  task_queue_manager_->SetTimeSourceForTesting(time_source);
}

void RendererSchedulerImpl::SetWorkBatchSizeForTesting(size_t work_batch_size) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  task_queue_manager_->SetWorkBatchSize(work_batch_size);
}

void RendererSchedulerImpl::SetLongIdlePeriodsEnabledForTesting(
    bool long_idle_periods_enabled) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  long_idle_periods_enabled_ = long_idle_periods_enabled;
}

base::TimeTicks RendererSchedulerImpl::Now() const {
  return UNLIKELY(time_source_) ? time_source_->Now() : base::TimeTicks::Now();
}

RendererSchedulerImpl::PollableNeedsUpdateFlag::PollableNeedsUpdateFlag(
    base::Lock* write_lock_)
    : flag_(false), write_lock_(write_lock_) {
}

RendererSchedulerImpl::PollableNeedsUpdateFlag::~PollableNeedsUpdateFlag() {
}

void RendererSchedulerImpl::PollableNeedsUpdateFlag::SetWhileLocked(
    bool value) {
  write_lock_->AssertAcquired();
  base::subtle::Release_Store(&flag_, value);
}

bool RendererSchedulerImpl::PollableNeedsUpdateFlag::IsSet() const {
  return base::subtle::Acquire_Load(&flag_) != 0;
}

// static
const char* RendererSchedulerImpl::TaskQueueIdToString(QueueId queue_id) {
  switch (queue_id) {
    case DEFAULT_TASK_QUEUE:
      return "default_tq";
    case COMPOSITOR_TASK_QUEUE:
      return "compositor_tq";
    case LOADING_TASK_QUEUE:
      return "loading_tq";
    case IDLE_TASK_QUEUE:
      return "idle_tq";
    case CONTROL_TASK_QUEUE:
      return "control_tq";
    case CONTROL_TASK_AFTER_WAKEUP_QUEUE:
      return "control_after_wakeup_tq";
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
const char* RendererSchedulerImpl::PolicyToString(Policy policy) {
  switch (policy) {
    case Policy::NORMAL:
      return "normal";
    case Policy::COMPOSITOR_PRIORITY:
      return "compositor";
    case Policy::TOUCHSTART_PRIORITY:
      return "touchstart";
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* RendererSchedulerImpl::InputStreamStateToString(
    InputStreamState state) {
  switch (state) {
    case InputStreamState::INACTIVE:
      return "inactive";
    case InputStreamState::ACTIVE:
      return "active";
    case InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE:
      return "active_and_awaiting_touchstart_response";
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* RendererSchedulerImpl::IdlePeriodStateToString(
    IdlePeriodState idle_period_state) {
  switch (idle_period_state) {
    case IdlePeriodState::NOT_IN_IDLE_PERIOD:
      return "not_in_idle_period";
    case IdlePeriodState::IN_SHORT_IDLE_PERIOD:
      return "in_short_idle_period";
    case IdlePeriodState::IN_LONG_IDLE_PERIOD:
      return "in_long_idle_period";
    case IdlePeriodState::IN_LONG_IDLE_PERIOD_WITH_MAX_DEADLINE:
      return "in_long_idle_period_with_max_deadline";
    case IdlePeriodState::ENDING_LONG_IDLE_PERIOD:
      return "ending_long_idle_period";
    default:
      NOTREACHED();
      return nullptr;
  }
}

scoped_refptr<base::trace_event::ConvertableToTraceFormat>
RendererSchedulerImpl::AsValueLocked(base::TimeTicks optional_now) const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  incoming_signals_lock_.AssertAcquired();

  if (optional_now.is_null())
    optional_now = Now();
  scoped_refptr<base::trace_event::TracedValue> state =
      new base::trace_event::TracedValue();

  state->SetString("current_policy", PolicyToString(current_policy_));
  state->SetString("idle_period_state",
                   IdlePeriodStateToString(idle_period_state_));
  state->SetString("input_stream_state",
                   InputStreamStateToString(input_stream_state_));
  state->SetDouble("now", (optional_now - base::TimeTicks()).InMillisecondsF());
  state->SetDouble("last_input_receipt_time_on_compositor_",
                   (last_input_receipt_time_on_compositor_ - base::TimeTicks())
                       .InMillisecondsF());
  state->SetDouble(
      "last_input_process_time_on_main_",
      (last_input_process_time_on_main_ - base::TimeTicks()).InMillisecondsF());
  state->SetDouble(
      "estimated_next_frame_begin",
      (estimated_next_frame_begin_ - base::TimeTicks()).InMillisecondsF());

  return state;
}

RendererSchedulerImpl::InputStreamState
RendererSchedulerImpl::ComputeNewInputStreamState(
    InputStreamState current_state,
    blink::WebInputEvent::Type new_input_type,
    blink::WebInputEvent::Type last_input_type) {
  switch (new_input_type) {
    case blink::WebInputEvent::TouchStart:
      return InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE;

    case blink::WebInputEvent::TouchMove:
      // Observation of consecutive touchmoves is a strong signal that the page
      // is consuming the touch sequence, in which case touchstart response
      // prioritization is no longer necessary. Otherwise, the initial touchmove
      // should preserve the touchstart response pending state.
      if (current_state ==
          InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE) {
        return last_input_type == blink::WebInputEvent::TouchMove
                   ? InputStreamState::ACTIVE
                   : InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE;
      }
      break;

    case blink::WebInputEvent::GestureTapDown:
    case blink::WebInputEvent::GestureShowPress:
    case blink::WebInputEvent::GestureFlingCancel:
    case blink::WebInputEvent::GestureScrollEnd:
      // With no observable effect, these meta events do not indicate a
      // meaningful touchstart response and should not impact task priority.
      return current_state;

    default:
      break;
  }
  return InputStreamState::ACTIVE;
}

void RendererSchedulerImpl::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (task_queue_manager_)
    task_queue_manager_->AddTaskObserver(task_observer);
}

void RendererSchedulerImpl::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (task_queue_manager_)
    task_queue_manager_->RemoveTaskObserver(task_observer);
}

}  // namespace content
