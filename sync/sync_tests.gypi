# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    # Test support files for the 'sync_core' target.
    # GN version: //sync:test_support_sync_core
    {
      'target_name': 'test_support_sync_core',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'include_dirs': [
        '..',
      ],
      'defines': [
        'SYNC_TEST'
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        'sync',
      ],
      'export_dependent_settings': [
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        'sync',
      ],
      'sources': [
        'internal_api/public/base/model_type_test_util.cc',
        'internal_api/public/base/model_type_test_util.h',
        'js/js_test_util.cc',
        'js/js_test_util.h',
        'sessions/test_util.cc',
        'sessions/test_util.h',
        'test/callback_counter.h',
        "test/directory_backing_store_corruption_testing.cc",
        "test/directory_backing_store_corruption_testing.h",
        'test/engine/fake_model_worker.cc',
        'test/engine/fake_model_worker.h',
        'test/engine/fake_sync_scheduler.cc',
        'test/engine/fake_sync_scheduler.h',
        'test/engine/injectable_sync_context_proxy.cc',
        'test/engine/injectable_sync_context_proxy.h',
        'test/engine/mock_connection_manager.cc',
        'test/engine/mock_connection_manager.h',
        'test/engine/mock_model_type_sync_proxy.cc',
        'test/engine/mock_model_type_sync_proxy.h',
        'test/engine/mock_model_type_sync_worker.cc',
        'test/engine/mock_model_type_sync_worker.h',
        'test/engine/mock_nudge_handler.cc',
        'test/engine/mock_nudge_handler.h',
        'test/engine/mock_update_handler.cc',
        'test/engine/mock_update_handler.h',
        'test/engine/single_type_mock_server.cc',
        'test/engine/single_type_mock_server.h',
        'test/engine/test_directory_setter_upper.cc',
        'test/engine/test_directory_setter_upper.h',
        'test/engine/test_id_factory.h',
        'test/engine/test_syncable_utils.cc',
        'test/engine/test_syncable_utils.h',
        'test/fake_encryptor.cc',
        'test/fake_encryptor.h',
        'test/fake_sync_encryption_handler.cc',
        'test/fake_sync_encryption_handler.h',
        'test/mock_invalidation.cc',
        'test/mock_invalidation.h',
        'test/mock_invalidation_tracker.cc',
        'test/mock_invalidation_tracker.h',
        'test/null_directory_change_delegate.cc',
        'test/null_directory_change_delegate.h',
        'test/null_transaction_observer.cc',
        'test/null_transaction_observer.h',
        'test/sessions/mock_debug_info_getter.cc',
        'test/sessions/mock_debug_info_getter.h',
        'test/sessions/test_scoped_session_event_listener.h',
        'test/test_directory_backing_store.cc',
        'test/test_directory_backing_store.h',
        'test/test_transaction_observer.cc',
        'test/test_transaction_observer.h',
        'test/trackable_mock_invalidation.cc',
        'test/trackable_mock_invalidation.h',
        'util/mock_unrecoverable_error_handler.cc',
        'util/mock_unrecoverable_error_handler.h',
        'util/test_unrecoverable_error_handler.cc',
        'util/test_unrecoverable_error_handler.h',
      ],
    },

    # Test support files for the python sync test server.
    # GN version: //sync:test_support_sync_testserver
    {
      'target_name': 'test_support_sync_testserver',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../net/net.gyp:net_test_support',
        # The sync test server uses Python modules generated by the sync protos.
        '../third_party/protobuf/protobuf.gyp:py_proto',
        'sync',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
        '../net/net.gyp:net_test_support',
      ],
      'sources': [
        'test/local_sync_test_server.cc',
        'test/local_sync_test_server.h',
      ],
    },

    # Test support files for the fake sync server.
    # GN version: //sync:test_support_sync_fake_server
    {
      'target_name': 'test_support_sync_fake_server',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../net/net.gyp:net',
        '../testing/gtest.gyp:gtest',
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
        'sync',
      ],
      'export_dependent_settings': [
        'sync',
      ],
      'sources': [
        'test/fake_server/bookmark_entity.cc',
        'test/fake_server/bookmark_entity.h',
        'test/fake_server/bookmark_entity_builder.cc',
        'test/fake_server/bookmark_entity_builder.h',
        'test/fake_server/entity_builder.cc',
        'test/fake_server/entity_builder.h',
        'test/fake_server/entity_builder_factory.cc',
        'test/fake_server/entity_builder_factory.h',
        'test/fake_server/fake_server.cc',
        'test/fake_server/fake_server.h',
        'test/fake_server/fake_server_entity.cc',
        'test/fake_server/fake_server_entity.h',
        'test/fake_server/fake_server_http_post_provider.cc',
        'test/fake_server/fake_server_http_post_provider.h',
        'test/fake_server/fake_server_network_resources.cc',
        'test/fake_server/fake_server_network_resources.h',
        'test/fake_server/fake_server_verifier.cc',
        'test/fake_server/fake_server_verifier.h',
        'test/fake_server/permanent_entity.cc',
        'test/fake_server/permanent_entity.h',
        'test/fake_server/sessions_hierarchy.cc',
        'test/fake_server/sessions_hierarchy.h',
        'test/fake_server/tombstone_entity.cc',
        'test/fake_server/tombstone_entity.h',
        'test/fake_server/unique_client_entity.cc',
        'test/fake_server/unique_client_entity.h',
      ],
    },

    # Test support files for the 'sync_internal_api' target.
    # GN version: //sync:test_support_sync_internal_api
    {
      'target_name': 'test_support_sync_internal_api',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'include_dirs': [
        '..',
      ],
      'defines': [
        'SYNC_TEST'
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../testing/gtest.gyp:gtest',
        'sync',
        'test_support_sync_core',
      ],
      'export_dependent_settings': [
        '../testing/gtest.gyp:gtest',
        'sync',
        'test_support_sync_core',
      ],
      'sources': [
        'internal_api/public/test/fake_sync_manager.h',
        'internal_api/public/test/null_sync_context_proxy.h',
        'internal_api/public/test/sync_manager_factory_for_profile_sync_test.h',
        'internal_api/public/test/test_entry_factory.h',
        'internal_api/public/test/test_internal_components_factory.h',
        'internal_api/public/test/test_user_share.h',
        'internal_api/test/fake_sync_manager.cc',
        'internal_api/test/null_sync_context_proxy.cc',
        'internal_api/test/sync_manager_factory_for_profile_sync_test.cc',
        'internal_api/test/sync_manager_for_profile_sync_test.cc',
        'internal_api/test/sync_manager_for_profile_sync_test.h',
        'internal_api/test/test_entry_factory.cc',
        'internal_api/test/test_internal_components_factory.cc',
        'internal_api/test/test_user_share.cc',
      ],
    },

    # Test support files for the 'sync_api' target.
    # GN version: //sync:test_support_sync_api
    {
      'target_name': 'test_support_sync_api',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'defines': [
        'SYNC_TEST'
      ],
      'dependencies': [
        '../testing/gmock.gyp:gmock',
        'sync',
      ],
      'export_dependent_settings': [
        '../testing/gmock.gyp:gmock',
        'sync',
      ],
      'sources': [
        'api/fake_sync_change_processor.cc',
        'api/fake_sync_change_processor.h',
        'api/fake_syncable_service.cc',
        'api/fake_syncable_service.h',
        'api/sync_change_processor_wrapper_for_test.cc',
        'api/sync_change_processor_wrapper_for_test.h',
        'api/sync_error_factory_mock.cc',
        'api/sync_error_factory_mock.h',
      ],
    },

    # The unit test executable for sync tests.
    # GN version: //sync:sync_unit_tests
    {
      'target_name': 'sync_unit_tests',
      'type': '<(gtest_target_type)',
      # Typed-parametrized tests generate exit-time destructors.
      'variables': { 'enable_wexit_time_destructors': 0, },
      'defines': [
        'SYNC_TEST',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:run_all_unittests',
        '../google_apis/google_apis.gyp:google_apis',
        '../google_apis/google_apis.gyp:google_apis_test_support',
        '../net/net.gyp:net',
        '../net/net.gyp:net_test_support',
        '../sql/sql.gyp:sql',
        '../sql/sql.gyp:test_support_sql',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
        'sync',
        'test_support_sync_core',
        'test_support_sync_internal_api',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'api/attachments/attachment_id_unittest.cc',
        'api/attachments/attachment_metadata_unittest.cc',
        'api/attachments/attachment_unittest.cc',
        'api/sync_change_unittest.cc',
        'api/sync_data_unittest.cc',
        'api/sync_error_unittest.cc',
        'api/sync_merge_result_unittest.cc',
        'engine/apply_control_data_updates_unittest.cc',
        'engine/backoff_delay_provider_unittest.cc',
        'engine/directory_commit_contribution_unittest.cc',
        'engine/directory_update_handler_unittest.cc',
        'engine/entity_tracker_unittest.cc',
        'engine/get_updates_processor_unittest.cc',
        'engine/model_type_entity_unittest.cc',
        'engine/model_type_sync_proxy_impl_unittest.cc',
        'engine/model_type_sync_worker_impl_unittest.cc',
        'engine/sync_scheduler_unittest.cc',
        'engine/syncer_proto_util_unittest.cc',
        'engine/syncer_unittest.cc',
        'engine/syncer_util_unittest.cc',
        'internal_api/attachments/attachment_downloader_impl_unittest.cc',
        'internal_api/attachments/attachment_service_impl_unittest.cc',
        'internal_api/attachments/attachment_service_proxy_unittest.cc',
        'internal_api/attachments/attachment_store_frontend_unittest.cc',
        'internal_api/attachments/attachment_store_test_template.h',
        'internal_api/attachments/attachment_uploader_impl_unittest.cc',
        'internal_api/attachments/fake_attachment_downloader_unittest.cc',
        'internal_api/attachments/fake_attachment_uploader_unittest.cc',
        'internal_api/attachments/in_memory_attachment_store_unittest.cc',
        'internal_api/attachments/on_disk_attachment_store_unittest.cc',
        'internal_api/attachments/task_queue_unittest.cc',
        'internal_api/debug_info_event_listener_unittest.cc',
        'internal_api/http_bridge_unittest.cc',
        'internal_api/js_mutation_event_observer_unittest.cc',
        'internal_api/js_sync_encryption_handler_observer_unittest.cc',
        'internal_api/js_sync_manager_observer_unittest.cc',
        'internal_api/protocol_event_buffer_unittest.cc',
        'internal_api/public/base/attachment_id_proto_unittest.cc',
        'internal_api/public/base/cancelation_signal_unittest.cc',
        'internal_api/public/base/enum_set_unittest.cc',
        'internal_api/public/base/node_ordinal_unittest.cc',
        'internal_api/public/base/ordinal_unittest.cc',
        'internal_api/public/base/unique_position_unittest.cc',
        'internal_api/public/change_record_unittest.cc',
        'internal_api/public/engine/model_safe_worker_unittest.cc',
        'internal_api/public/sessions/sync_session_snapshot_unittest.cc',
        'internal_api/public/util/immutable_unittest.cc',
        'internal_api/public/util/weak_handle_unittest.cc',
        'internal_api/sync_backup_manager_unittest.cc',
        'internal_api/sync_context_proxy_impl_unittest.cc',
        'internal_api/sync_encryption_handler_impl_unittest.cc',
        'internal_api/sync_manager_impl_unittest.cc',
        'internal_api/sync_rollback_manager_base_unittest.cc',
        'internal_api/sync_rollback_manager_unittest.cc',
        'internal_api/syncapi_server_connection_manager_unittest.cc',
        'js/js_event_details_unittest.cc',
        'js/sync_js_controller_unittest.cc',
        'protocol/proto_enum_conversions_unittest.cc',
        'protocol/proto_value_conversions_unittest.cc',
        'sessions/model_type_registry_unittest.cc',
        'sessions/nudge_tracker_unittest.cc',
        'sessions/status_controller_unittest.cc',
        'syncable/deferred_on_disk_directory_backing_store_unittest.cc',
        'syncable/directory_backing_store_unittest.cc',
        'syncable/directory_unittest.cc',
        'syncable/directory_unittest.h',
        'syncable/entry_kernel_unittest.cc',
        'syncable/model_type_unittest.cc',
        'syncable/nigori_util_unittest.cc',
        'syncable/parent_child_index_unittest.cc',
        'syncable/syncable_enum_conversions_unittest.cc',
        'syncable/syncable_id_unittest.cc',
        'syncable/syncable_unittest.cc',
        'syncable/syncable_util_unittest.cc',
        'util/cryptographer_unittest.cc',
        'util/data_type_histogram_unittest.cc',
        'util/get_session_name_unittest.cc',
        'util/nigori_unittest.cc',
        'util/protobuf_unittest.cc',
      ],
      'conditions': [
        # TODO(akalin): This is needed because histogram.cc uses
        # leak_annotations.h, which pulls this in.  Make 'base'
        # propagate this dependency.
        ['OS=="linux" and use_allocator!="none"', {
          'dependencies': [
            '../base/allocator/allocator.gyp:allocator',
          ],
        }],
        ['OS == "android"', {
          'dependencies': [
            '../testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['OS=="linux" and chromeos==1', {
          # Required by get_session_name_unittest.cc on Chrome OS.
          'dependencies': [
            '../chromeos/chromeos.gyp:chromeos',
          ],
        }],
        ['OS == "ios"', {
          'sources!': [
            'internal_api/http_bridge_unittest.cc',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        # A tool that can be used to launch a python sync server instance.
        {
          'target_name': 'run_sync_testserver',
          'type': 'executable',
          'dependencies': [
            '../base/base.gyp:base',
            '../base/base.gyp:test_support_base',
            '../net/net.gyp:net_test_support',
            '../testing/gtest.gyp:gtest',
            'test_support_sync_testserver',
          ],
          'sources': [
            'tools/testserver/run_sync_testserver.cc',
          ],
        },
      ],
    }],
    ['OS == "android"', {
      'targets': [
        {
          # GN: //sync/android:sync_javatests
          'target_name': 'sync_javatests',
          'type': 'none',
          'variables': {
            'java_in_dir': '../sync/android/javatests',
          },
          'dependencies': [
            'sync_java',
            'sync_java_test_support',
            '../base/base.gyp:base_java_test_support',
          ],
          'includes': [ '../build/java.gypi' ],
        },
        {
          # GN: //sync:sync_java_test_support
          'target_name': 'sync_java_test_support',
          'type': 'none',
          'variables': {
            'java_in_dir': '../sync/test/android/javatests',
          },
          'dependencies': [
            'sync_java',
          ],
          'includes': [ '../build/java.gypi' ],
        },
        {
          # GN: //sync:sync_fake_server_jni_headers
          'target_name': 'sync_fake_server_jni_headers',
          'type': 'none',
          'sources': [
            '../chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/FakeServerHelper.java',
          ],
          'variables': {
            'jni_gen_package': 'sync/test/fake_server',
            'jni_generator_ptr_type': 'long',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },
        {
          # TODO(pvalenzuela): Create GN version of this target.
          # http://crbug.com/475612
          'target_name': 'test_support_sync_proto_java',
          'type': 'none',
          'variables': {
            'proto_in_dir': '<(INTERMEDIATE_DIR)/sync_protos',
            # Set this variable so that sync_proto_source_paths refers to the
            # temporary proto definitions created here.
            'sync_proto_sources_dir': '<(INTERMEDIATE_DIR)/sync_protos',
          },
          'actions': [
            {
              'action_name': 'run_script',
              'inputs': [
                'protocol/prepare_protos_for_java_tests.py',
                # Use the original list of proto files (defined in sync.gyp).
                '<@(sync_proto_sources)',
              ],
              'outputs': [
                '<@(sync_proto_source_paths)',
              ],
              'action': [
                'python',
                'protocol/prepare_protos_for_java_tests.py',
                '--output_dir',
                '<(INTERMEDIATE_DIR)/sync_protos',
                # Use the original list of proto files (defined in sync.gyp).
                '<@(sync_proto_sources)'
              ],
            },
          ],
          'sources': [
            '<@(sync_proto_source_paths)',
          ],
          'includes': ['protocol/protocol.gypi', '../build/protoc_java.gypi'],
        },
        {
          # GN: //sync:test_support_sync_fake_server_android
          'target_name': 'test_support_sync_fake_server_android',
          'type': 'static_library',
          'dependencies': [
            'sync_fake_server_jni_headers',
            'test_support_sync_fake_server',
            '../testing/gtest.gyp:gtest',
            '../base/base.gyp:base',
          ],
          'export_dependent_settings': [
            '../testing/gtest.gyp:gtest',
          ],
          'sources': [
            'test/fake_server/android/fake_server_helper_android.cc',
            'test/fake_server/android/fake_server_helper_android.h',
          ],
        },
        {
          # GN: //sync:sync_unit_tests_apk
          'target_name': 'sync_unit_tests_apk',
          'type': 'none',
          'dependencies': [
            'sync_unit_tests',
          ],
          'variables': {
            'test_suite_name': 'sync_unit_tests',
          },
          'includes': [ '../build/apk_test.gypi' ],
        },
      ],
    }],
  ],
}
