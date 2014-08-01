// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['../testing/chromevox_unittest_base.js']);

/**
 * Test fixture.
 * @constructor
 * @extends {ChromeVoxUnitTestBase}
 */
function CvoxDomUtilUnitTest() {}

CvoxDomUtilUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.ChromeVox',
    'cvox.DescriptionUtil',
    'cvox.DomUtil',
    'cvox.TestMsgs',
  ],

  /** @override */
  setUp: function() {
    cvox.ChromeVox.msgs = new cvox.TestMsgs();
  },

  asText_: function(node) {
    var temp = document.createElement('div');
    temp.appendChild(node);
    return temp.innerHTML;
  },

  assertEqualsAsText_: function(node1, node2) {
    assertEquals(this.asText_(node1), this.asText_(node2));
  },

  loadDomUtilTestDoc_: function() {
    this.loadDoc(function() {/*!
    <style type="text/css">
      #display_none { display: none; }
      #visibility_hidden { visibility: hidden; }
      #forced_visible { visibility: hidden; }
      #visibility_collapse { visibility: collapse; }
      #opacity_zero { opacity: 0; }
      #opacity_partial { opacity: 0.5; }
      #opacity_undefined { }
      #nested_visibility_hide { visibility: hidden; }
      #nested_visibility_show { visibility: visible; }
      #nested_display_none { display: none; }
      #nested_display_block { display: block; }
    </style>
   <form action="">

    <div id="normal_node">1</div>
    <div id="display_none">2</div>
    <div id="visibility_hidden">3</div>
    <div id="visibility_collapse">3b</div>
    <div id="opacity_zero">4</div>
    <div id="opacity_partial">4b</div>
    <div id="opacity_undefined">5</div>
    <select id="select_node"><option>5</option></select>
    <textarea id="textarea">6</textarea>
    <div id="forced_visible" aria-hidden="false">7</div>
    <p id="normal_para">----</p>
    <p id="presentation" role="presentation">----</p>
    <p id="aria_hidden" aria-hidden="true">----</p>
    <p id="only_spaces">    </p>
    <p id="only_tabs">        </p>
    <p id="only_newlines">

    </p>
    <p id="only_nbsp">&nbsp;</p>
    <p id="other_entity">&amp;</p>
    <img id="img">
    <img id="img_alt" alt="tree">
    <img id="img_blankalt" alt="">

    <input id="check" type="checkbox">
    <input id="check_checked" type="checkbox" checked>

    <span><p id="a">a</p></span>
    <span><p id="b">b</p><p id="c">c</p></span>
    </form>

    <a id="special_link1" href="http://google.com"><span id="empty_span"></span>
    </a>
    <a id="special_link2" href="http://google.com"><span>Text content</span></a>
    <a id="special_link3"><span></span></a>

    <div id="nested_visibility_hide">
      hide<div id="nested_visibility_show">show</div>me
    </div>
    <div id="nested_display_none">
      nothing<div id="nested_display_block">will</div>show
    </div>
    */});
  },
};

TEST_F('CvoxDomUtilUnitTest', 'IsVisible', function() {
  this.loadDomUtilTestDoc_();

  // Simple tests.
  var node = $('normal_node');
  assertEquals(true, cvox.DomUtil.isVisible(node));
  node = $('display_none');
  assertEquals(false, cvox.DomUtil.isVisible(node));
  node = $('visibility_hidden');
  assertEquals(false, cvox.DomUtil.isVisible(node));
  node = $('visibility_collapse');
  assertEquals(false, cvox.DomUtil.isVisible(node));
  node = $('opacity_zero');
  assertEquals(false, cvox.DomUtil.isVisible(node));
  node = $('opacity_partial');
  assertEquals(true, cvox.DomUtil.isVisible(node));
  node = $('opacity_undefined');
  assertEquals(true, cvox.DomUtil.isVisible(node));
  node = $('forced_visible');
  assertEquals(true, cvox.DomUtil.isVisible(node));

  // Nested visibility tests.
  node = $('nested_visibility_hide');
  assertEquals(true, cvox.DomUtil.isVisible(node)); // Has visible child.
  node = $('nested_visibility_hide').childNodes[0];
  assertEquals(false, cvox.DomUtil.isVisible(node)); // TextNode is invisible.
  node = $('nested_visibility_show');
  assertEquals(true, cvox.DomUtil.isVisible(node));
  node = $('nested_visibility_show').childNodes[0];
  assertEquals(true, cvox.DomUtil.isVisible(node)); // TextNode is visible.
  node = $('nested_display_block');
  assertEquals(false, cvox.DomUtil.isVisible(node));

  // Options tests (for performance).
  node = $('nested_display_block');
  assertEquals(true,
      cvox.DomUtil.isVisible(node, {checkAncestors: false}));
  node = $('nested_visibility_hide');
  assertEquals(false,
      cvox.DomUtil.isVisible(node, {checkDescendants: false}));
});

/** Test determining if a node is a leaf node or not. @export */
TEST_F('CvoxDomUtilUnitTest', 'IsLeafNode', function() {
  this.loadDomUtilTestDoc_();

  var node = $('normal_node');
  assertEquals(false, cvox.DomUtil.isLeafNode(node));
  node = $('display_none');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('visibility_hidden');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('opacity_zero');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('select_node');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('textarea');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('normal_para');
  assertEquals(false, cvox.DomUtil.isLeafNode(node));
  node = $('aria_hidden');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('special_link1');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('special_link2');
  assertEquals(true, cvox.DomUtil.isLeafNode(node));
  node = $('special_link3');
  assertEquals(false, cvox.DomUtil.isLeafNode(node));
  node = $('nested_visibility_hide');
  assertEquals(false, cvox.DomUtil.isLeafNode(node));
});

/** Test determining if a node has content or not. @export */
TEST_F('CvoxDomUtilUnitTest', 'HasContent', function() {
  this.loadDomUtilTestDoc_();

  var node = $('normal_node');
  cvox.DomUtil.hasContent(node);
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('display_none');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('visibility_hidden');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('opacity_zero');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('select_node');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('textarea');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('normal_para');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  // TODO (adu): This test fails. Will inspect.
  // node = $('presentation');
  // assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('aria_hidden');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('only_spaces');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('only_tabs');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('only_newlines');
  assertEquals(false, cvox.DomUtil.hasContent(node));
  node = $('other_entity');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('img');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('img_alt');
  assertEquals(true, cvox.DomUtil.hasContent(node));
  node = $('img_blankalt');
  assertEquals(false, cvox.DomUtil.hasContent(node));
});

/** Test getting a node's state. @export */
TEST_F('CvoxDomUtilUnitTest', 'NodeState', function() {
  this.loadDomUtilTestDoc_();
  this.appendDoc(function() {/*!
  <input id="state1_enabled">
  <input id="state1_disabled" disabled>
  <button id="state2_enabled">Button</button>
  <button id="state2_disabled" disabled>Button</button>
  <textarea id="state3_enabled">Textarea</textarea>
  <textarea id="state3_disabled" disabled>Textarea</textarea>
  <select id="state4_enabled"><option>Select</option></select>
  <select id="state4_disabled" disabled><option>Select</option></select>
  <div role="button" id="state5_enabled" tabindex="0">ARIAButton</div>
  <div role="button" id="state5_disabled" tabindex="0" disabled>ARIAButton</div>
  <fieldset>
    <input id="state6_enabled">
  </fieldset>
  <fieldset disabled>
    <input id="state6_disabled">
  </fieldset>
  */});
  var node = $('check');
  assertEquals('not checked', cvox.DomUtil.getState(node, true));
  node = $('check_checked');
  assertEquals('checked', cvox.DomUtil.getState(node, true));
  node = $('state1_enabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state1_disabled');
  assertEquals('Disabled', cvox.DomUtil.getState(node, true));
  node = $('state2_enabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state2_disabled');
  assertEquals('Disabled', cvox.DomUtil.getState(node, true));
  node = $('state3_enabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state3_disabled');
  assertEquals('Disabled', cvox.DomUtil.getState(node, true));
  node = $('state4_enabled');
  assertEquals('1 of 1', cvox.DomUtil.getState(node, true));
  node = $('state4_disabled');
  assertEquals('1 of 1 Disabled', cvox.DomUtil.getState(node, true));
  node = $('state5_enabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state5_disabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state6_enabled');
  assertEquals('', cvox.DomUtil.getState(node, true));
  node = $('state6_disabled');
  assertEquals('Disabled', cvox.DomUtil.getState(node, true));
});

/** Test finding the next/previous leaf node. @export */
TEST_F('CvoxDomUtilUnitTest', 'LeafNodeTraversal', function() {
  this.loadDomUtilTestDoc_();

  var node = $('a');
  node = cvox.DomUtil.directedNextLeafNode(node);
  assertEquals('\n    ', node.textContent);
  node = cvox.DomUtil.directedNextLeafNode(node);
  assertEquals('b', node.textContent);
  node = cvox.DomUtil.directedNextLeafNode(node);
  assertEquals('c', node.textContent);
  node = cvox.DomUtil.previousLeafNode(node);
  assertEquals('b', node.textContent);
  node = cvox.DomUtil.previousLeafNode(node);
  assertEquals('\n    ', node.textContent);
  node = cvox.DomUtil.previousLeafNode(node);
  assertEquals('a', node.textContent);
});

/** Test finding the label for controls. @export */
TEST_F('CvoxDomUtilUnitTest', 'GetLabel', function() {
  this.loadDoc(function() {/*!
    <fieldset id="Fieldset">
    <legend>This is a legend inside a fieldset</legend>
    <div align="right">
    <span>
    Username:
    </span>
    </div>
    <input name="Email" id="Email" size="18" value="" type="text">
    <span>
    Password:
    </span>
    <input name="Passwd" id="Passwd" size="18" type="password">
    <input name="PersistentCookie" id="PersistentCookie" type="checkbox">
    <label for="PersistentCookie" id="PersistentCookieLabel">
    Stay signed in
    </label>
    <input name="signIn" id="signIn" value="Sign in" type="submit">
    <input id="dummyA" size="18" value="" type="text" title="">
    <input id="dummyB" size="18" value="" type="text" aria-label="">
    </fieldset>
  */});

  function getControlText(control) {
    var description = cvox.DescriptionUtil.getControlDescription(control);
    return cvox.DomUtil.collapseWhitespace(
        description.context + ' ' +
        description.text + ' ' +
        description.userValue + ' ' +
        description.annotation);
  }

  var fieldsetElement = $('Fieldset');
  assertEquals('This is a legend inside a fieldset',
      cvox.DomUtil.getName(fieldsetElement, false, false));

  var usernameField = $('Email');
  assertEquals('', cvox.DomUtil.getValue(usernameField));
  assertEquals('Username:',
      cvox.DomUtil.getControlLabelHeuristics(usernameField));
  assertEquals('Username: Edit text', getControlText(usernameField));
  var passwordField = $('Passwd');
  assertEquals('', cvox.DomUtil.getValue(passwordField));
  assertEquals('Password:',
      cvox.DomUtil.getControlLabelHeuristics(passwordField));
  assertEquals('Password: Password edit text', getControlText(passwordField));
  var cookieCheckbox = $('PersistentCookie');
  assertEquals('Stay signed in', cvox.DomUtil.getName(cookieCheckbox));
  assertEquals('Stay signed in Check box not checked',
      getControlText(cookieCheckbox));
  var signinButton = $('signIn');
  assertEquals('Sign in', cvox.DomUtil.getName(signinButton));
  assertEquals('Sign in Button', getControlText(signinButton));
  var dummyInputA = $('dummyA');
  assertEquals('', cvox.DomUtil.getName(dummyInputA));
  var dummyInputB = $('dummyB');
  assertEquals('', cvox.DomUtil.getName(dummyInputB));

  // The heuristic no longer returns 'Stay signed in' as the label for
  // the signIn button because 'Stay signed in' is in a label that's
  // explicitly associated with another control.
  //assertEquals('Stay signed in ',
  //    cvox.DomUtil.getControlLabelHeuristics(signinButton));
});

/** Test finding the label for controls with a more complex setup. @export */
TEST_F('CvoxDomUtilUnitTest', 'GetLabelComplex', function() {
  this.loadDoc(function() {/*!
  <table class="bug-report-table">
  <tbody><tr>
  <td class="bug-report-fieldlabel">
  <input id="page-url-checkbox" type="checkbox">
  <span id="page-url-label" i18n-content="page-url">Include this URL:</span>
  </td>
  <td>
  <input id="page-url-text" class="bug-report-field" maxlength="200">
  </td>
  </tr>
  </tbody></table>
  <table id="user-email-table" class="bug-report-table">
  <tbody><tr>
  <td class="bug-report-fieldlabel">
  <input id="user-email-checkbox" checked="checked" type="checkbox">
  <span id="user-email-label">Include this email:</span>
  </td>
  <td>
  <label id="user-email-text" class="bug-report-field"></label>
  </td>
  </tr>
  </tbody></table>
  <table class="bug-report-table">
  <tbody><tr>
  <td class="bug-report-fieldlabel">
  <input id="sys-info-checkbox" checked="checked" type="checkbox">
  <span id="sysinfo-label">
  <a id="sysinfo-url" href="#">Send system information</a>
  </span>
  </td>
  </tr>
  </tbody></table>
  <table class="bug-report-table">
  <tbody><tr>
  <td class="bug-report-fieldlabel">
  <input id="screenshot-checkbox" type="checkbox">
  <span id="screenshot-label-current">Include the current screenshot:</span>
  </td>
  </tr>
  </tbody></table>
  */});
  var urlCheckbox = $('page-url-checkbox');
  assertEquals('Include this URL:',
      cvox.DomUtil.getControlLabelHeuristics(urlCheckbox));
  var emailCheckbox = $('user-email-checkbox');
  assertEquals('Include this email:',
      cvox.DomUtil.getControlLabelHeuristics(emailCheckbox));
  var sysCheckbox = $('sys-info-checkbox');
  assertEquals('Send system information',
      cvox.DomUtil.getControlLabelHeuristics(sysCheckbox));
});

/**************************************************************/

TEST_F('CvoxDomUtilUnitTest', 'EscapedNames', function() {
  this.loadDoc(function() {/*!
     <p id="en-title" title="&lt;&gt;"></p>
     <p id="en-arialabel" aria-label="&lt;&gt;"></p>
     <img id="en-img" title="&lt;&gt;"></img>
     <p id="en-double" title="&amp;lt;&amp;gt;"></p>
     */});
  assertEquals('<>', cvox.DomUtil.getName(
      $('en-title')));
  assertEquals('<>', cvox.DomUtil.getName(
      $('en-arialabel')));
  assertEquals('<>', cvox.DomUtil.getName(
      $('en-img')));
  assertEquals('&lt;&gt;', cvox.DomUtil.getName(
      $('en-double')));
});

/** Test a paragraph with plain text. @export */
TEST_F('CvoxDomUtilUnitTest', 'SimplePara', function() {
  this.loadDoc(function() {/*!
    <p id="simplepara">This is a simple paragraph.</p>
  */});
  var node = $('simplepara');
  var text = cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(node));
  assertEquals('This is a simple paragraph.', text);
});

/** Test a paragraph with nested tags. @export */
TEST_F('CvoxDomUtilUnitTest', 'NestedPara', function() {
  this.loadDoc(function() {/*!
    <p id="nestedpara">This is a <b>paragraph</b> with <i>nested</i> tags.</p>
  */});
  var node = $('nestedpara');
  var text = cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(node));
  assertEquals('This is a paragraph with nested tags.', text);
});

/**
 * Test a paragraph with nested tags and varying visibility.
 * @export
 */
TEST_F('CvoxDomUtilUnitTest', 'NestedVisibilityPara', function() {
  this.loadDoc(function() {/*!
    <style type="text/css">
      #nested_visibility_paragraph { }
      #nested_visibility_paragraph .hide { visibility: hidden; }
      #nested_visibility_paragraph .show { visibility: visible; }
    </style>
    <p id="nested_visibility_paragraph">
      This is
      <span class="hide">
        not
        <span class="show"> a sentence.</span>
      </span>
    </p>
  */});
  var node = $('nested_visibility_paragraph');
  var text = cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(node));
  assertEquals('This is a sentence.', text);
});

/** Test getting text from an IMG node. @export */
TEST_F('CvoxDomUtilUnitTest', 'Image', function() {
  this.loadDoc(function() {/*!
    <img id="img">
    <img id="img_noalt" src="rose.png">
    <img id="img_alt" alt="flower" src="rose.png">
    <img id="img_title" title="a Flower" src="rose.png">
    <img id="img_noalt_long"
         src="777777777777777777777777777777777.png">
  */});

  var node = $('img');
  assertEquals('Image', cvox.DomUtil.getName(node));
  node = $('img_noalt');
  assertEquals('rose Image', cvox.DomUtil.getName(node));
  node = $('img_alt');
  assertEquals('flower', cvox.DomUtil.getName(node));
  node = $('img_title');
  assertEquals('a Flower', cvox.DomUtil.getName(node));
  node = $('img_noalt_long');
  assertEquals('Image', cvox.DomUtil.getName(node));
});

/** Test getting text from a select box. @export */
TEST_F('CvoxDomUtilUnitTest', 'Select', function() {
  this.loadDoc(function() {/*!
    <select id="select_noneselected">
      <option>Apple</option>
      <option>Banana</option>
      <option>Pear</option>
    </select>
    <select id="select_bananaselected">
      <option>Apple</option>
      <option selected>Banana</option>
      <option>Pear</option>
    </select>
  */});

  $('select_noneselected').selectedIndex = -1;
  var node = $('select_noneselected');
  assertEquals('', cvox.DomUtil.getValue(node));
  node = $('select_bananaselected');
  assertEquals('Banana', cvox.DomUtil.getValue(node));
});

/** Test whether funky html causes getName to go into infinite loop. */
TEST_F('CvoxDomUtilUnitTest', 'GetNameInfiniteLoop', function() {
  this.loadDoc(function() {/*!
    <div>
      <label for="a">
        <p id="a">asdf</p>
      </label>
    </div>
  */});
  // intentionally no asserts; if there is an infinite (recursive) loop,
  // the stack will blow up
  var node = $('a');
  var label = cvox.DomUtil.getName(node);
});

/** Test getting text from an INPUT control. @export */
TEST_F('CvoxDomUtilUnitTest', 'Input', function() {
  this.loadDoc(function() {/*!
    <form action="">
      <input id="hidden" type="hidden" value="hidden1">
      <input id="input_img" type="image" src="rose.png">
      <input id="input_img_alt" type="image" alt="flower" src="rose.png">
      <input id="submit" type="submit">
      <input id="submit_withvalue" type="submit" value="Go">
      <input id="reset" type="reset">
      <input id="reset_withvalue" type="reset" value="Stop">
      <input id="button" type="button" value="Button">
      <input id="checkbox" type="checkbox" value="ignore1">
      <input id="checkbox_title" type="checkbox" value="ignore1" title="toggle">
      <input id="radio" type="radio" value="ignore2">
      <input id="password" type="password" value="dragon">
      <input id="text" value="my text">
      <input id="placeholder0" placeholder="Phone number">
      <input id="placeholder1" title="Phone number">
      <input id="placeholder2" title="Phone number" placeholder="xxx-yyy-zzzz">
      <input id="placeholder3" title="Phone number" placeholder="xxx-yyy-zzzz"
                               value="310-555-1212">
    </form>
  */});

  var node = $('hidden');
  assertEquals('', cvox.DomUtil.getName(node));
  node = $('input_img');
  assertEquals('rose Image', cvox.DomUtil.getName(node));
  node = $('input_img_alt');
  assertEquals('flower', cvox.DomUtil.getName(node));
  node = $('submit');
  assertEquals('Submit', cvox.DomUtil.getName(node));
  node = $('submit_withvalue');
  assertEquals('Go', cvox.DomUtil.getName(node));
  node = $('reset');
  assertEquals('Reset', cvox.DomUtil.getName(node));
  node = $('reset_withvalue');
  assertEquals('Stop', cvox.DomUtil.getName(node));
  node = $('button');
  assertEquals('Button', cvox.DomUtil.getName(node));
  node = $('checkbox');
  assertEquals('', cvox.DomUtil.getName(node));
  node = $('checkbox_title');
  assertEquals('toggle', cvox.DomUtil.getName(node));
  node = $('radio');
  assertEquals('', cvox.DomUtil.getName(node));
  node = $('password');
  assertEquals('dot dot dot dot dot dot ', cvox.DomUtil.getValue(node));
  node = $('text');
  assertEquals('my text', cvox.DomUtil.getValue(node));
  node = $('placeholder0');
  assertEquals('Phone number', cvox.DomUtil.getName(node));
  node = $('placeholder1');
  assertEquals('Phone number', cvox.DomUtil.getName(node));
  node = $('placeholder2');
  assertEquals('xxx-yyy-zzzz',
               cvox.DomUtil.getName(node));
  node = $('placeholder3');
  assertEquals('310-555-1212 xxx-yyy-zzzz',
               cvox.DomUtil.getValue(node) + ' ' + cvox.DomUtil.getName(node));
});


/** Test checking if something is a control. @export */
TEST_F('CvoxDomUtilUnitTest', 'IsControl', function() {
  this.loadDoc(function() {/*!
  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td>&nbsp;</td>

        <td nowrap="nowrap">
          <table width="100%" border="0" cellpadding="0" cellspacing="0">
            <tbody>
              <tr>
                <td bgcolor="#3366CC"><img alt="" width="1" height="1"></td>
              </tr>
            </tbody>
          </table>

          <table width="100%" border="0" cellpadding="0" cellspacing="0">
            <tbody>
              <tr>
                <td bgcolor="#E5ECF9" nowrap="nowrap"><font color="#000000"
                face="arial,sans-serif" size="+1"><b>&nbsp;Preferences</b>
                </font></td>

                <td align="right" bgcolor="#E5ECF9" nowrap="nowrap">
                <font color="#000000" face="arial,sans-serif" size="-1">
                <a href="http://www.google.com/accounts/ManageAccount">Google
                Account settings</a> | <a href="http://www.google.com/">
                Preferences Help</a> | <a href="/about.html">About
                Google</a>&nbsp;</font></td>
              </tr>
            </tbody>
          </table>
        </td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="2" cellspacing="0">
    <tbody>
      <tr bgcolor="#E5ECF9">
        <td><font face="arial,sans-serif" size="-1"><b>Save</b> your
        preferences when finished and <b>return to search</b>.</font></td>

        <td align="right"><font face="arial,sans-serif" size="-1">
        <input value="Save Preferences " name="submit2" type="submit">
        </font></td>
      </tr>
    </tbody>
  </table>

  <h1>Global Preferences</h1><font size="-1">(changes apply to all Google
  services)</font><br>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td bgcolor="#CBDCED"><img alt="" width="1" height="2"></td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Interface Language</h2>
        </td>

        <td colspan="2"><br>
        <font face="arial,sans-serif" size="-1">Display Google tips and
        messages in: <select name="hl">
          <option value="af">
            Afrikaans
          </option>

          <option value="ak">
            Akan
          </option>

          <option value="sq">
            Albanian
          </option>

          <option value="am">
            Amharic
          </option>

          <option value="ar">
            Arabic
          </option>
        </select><br>
        If you do not find your native language in the pulldown above, you
        can<br>
        help Google create it through our
        <a href="http://services.google.com/">Google in Your Language
        program</a>.<br>
        &nbsp;</font></td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Search Language</h2>
        </td>

        <td>
          &nbsp;<br>
          <font face="arial,sans-serif" size="-1">Prefer pages written in these
          language(s):</font><br>

          <table border="0" cellpadding="5" cellspacing="10">
            <tbody>
              <tr>
                <td valign="top" nowrap="nowrap"><font face="arial,sans-serif"
                size="-1"><label><input name="lr" value="lang_af"
                onclick="tick()" id="paf" type="checkbox">
                <span id="taf">Afrikaans</span></label><br>
                <label><input name="lr" value="lang_ar" onclick="tick()"
                id="par" type="checkbox"> <span id="tar">Arabic</span></label>
                <br>
                <label><input name="lr" value="lang_hy" onclick="tick()"
                id="phy" type="checkbox"> <span id="thy">Armenian</span>
                </label><br>
                <label><input name="lr" value="lang_be" onclick="tick()"
                id="pbe" type="checkbox"> <span id="tbe">Belarusian</span>
                </label><br>
                <label><input name="lr" value="lang_bg" onclick="tick()"
                id="pbg" type="checkbox"> <span id="tbg">Bulgarian</span>
                </label><br>
                <label><input name="lr" value="lang_ca" onclick="tick()"
                id="pca" type="checkbox"> <span id="tca">Catalan</span>
                </label><br>
                <label><input name="lr" value="lang_zh-CN" onclick="tick()"
                id="pzh-CN" type="checkbox"> <span id="tzh-CN">
                Chinese&nbsp;(Simplified)</span></label><br>
                <label><input name="lr" value="lang_zh-TW" onclick="tick()"
                id="pzh-TW" type="checkbox"> <span id="tzh-TW">
                Chinese&nbsp;(Traditional)</span></label><br>
                <label><input name="lr" value="lang_hr" onclick="tick()"
                id="phr" type="checkbox"> <span id="thr">Croatian</span>
                </label><br>
                <label><input name="lr" value="lang_cs" onclick="tick()"
                id="pcs" type="checkbox"> <span id="tcs">Czech</span>
                </label><br>
                <label><input name="lr" value="lang_da" onclick="tick()"
                id="pda" type="checkbox"> <span id="tda">Danish</span>
                </label><br>
                <label><input name="lr" value="lang_nl" onclick="tick()"
                id="pnl" type="checkbox"> <span id="tnl">Dutch</span>
                </label></font></td>
              </tr>
            </tbody>
          </table>
        </td>
      </tr>
    </tbody>
  </table><a name="loc" id="loc"></a>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Location</h2>
        </td>

        <td>
          <br>

          <div style="color: rgb(204, 0, 0); display: none;" id="locerr">
            <span id="lem"><font face="arial,sans-serif" size="-1">The location
            <b>X</b> was not recognized.</font></span>
            <font face="arial,sans-serif" size="-1"><br>
            <br>
            Suggestions:<br></font>

            <ul>
              <li><font face="arial,sans-serif" size="-1">Make sure all street
              and city names are spelled correctly.</font></li>

              <li><font face="arial,sans-serif" size="-1">Make sure the address
              included a city and state.</font></li>

              <li><font face="arial,sans-serif" size="-1">Try entering a Zip
              code.</font></li>
            </ul>
          </div>

          <div style="color: rgb(204, 0, 0); display: none;" id="locterr">
            <font face="arial,sans-serif" size="-1">Please enter a valid US
            city or zip code<br>
            <br></font>
          </div>

          <div style="color: rgb(204, 0, 0); display: none;" id="locserr">
            <font face="arial,sans-serif" size="-1">Server error. Please try
            again.<br>
            <br></font>
          </div><font face="arial,sans-serif" size="-1">Use as the default
          location in Google Maps, customized search results, and other Google
          products:<br>
          <input name="uulo" value="1" type="hidden"><input name="muul"
          value="4_20" type="hidden"><input name="luul" size="60" value=""
          type="text"><br>
          This location is saved on this computer.
          <a href="/support/websearch/bin/answer.py?answer=35892&amp;hl=en">
          Learn more</a><br>
          <br></font>
        </td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td rowspan="2" width="1" bgcolor="#CBDCED">
        <img alt="" width="2" height="1"></td>

        <td width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;
          <h2>SafeSearch Filtering</h2>
        </td>
        <td><br>
        <font face="arial,sans-serif" size="-1">
        <a href="http://www.google.com/">
        Google's SafeSearch</a> blocks web pages containing explicit sexual
        content from appearing in search results.</font></td>
      </tr>
      <tr valign="top">
        <td width="175" nowrap="nowrap">&nbsp;</td>
        <td>
          <div style="margin-bottom: 1.2em; font: smaller arial,sans-serif;">
            <input id="stf" name="safeui" value="on" type="radio">
            <label for="stf">Use strict filtering&nbsp;(Filter both explicit
            text and explicit images)</label><br>
            <input id="modf" name="safeui" value="images" checked="checked"
            type="radio"><label for="modf">Use moderate
            filtering&nbsp;(Filter explicit images only - default
            behavior)</label><br>
            <input id="nof" name="safeui" value="off" type="radio">
            <label for="nof">Do not filter my search results</label>
          </div>
          <p style="margin-bottom: 1.2em; font-size: smaller;">This will apply
          strict filtering to all searches from this computer using Firefox.
          <a href="http://www.google.com/">Learn more</a></p>
        </td>
      </tr>
    </tbody>
  </table>
  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Number of Results</h2>
        </td>

        <td>&nbsp;<br>
        <font face="arial,sans-serif" size="-1">Google's default (10 results)
        provides the fastest results.<br>
        Display <select name="num">
          <option value="10" selected="selected">
            10
          </option>

          <option value="20">
            20
          </option>

          <option value="30">
            30
          </option>

          <option value="50">
            50
          </option>

          <option value="100">
            100
          </option>
        </select> results per page.<br>
        &nbsp;</font></td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Results Window</h2><a name="safeui" id="safeui">&nbsp;</a>
        </td>

        <td>&nbsp;<br>
        <font face="arial,sans-serif" size="-1"><input id="nwc" name="newwindow"
        value="1" type="checkbox">&nbsp; <label for="nwc">Open
        search results in a new browser window.</label></font><br>
        &nbsp;</td>
      </tr>
    </tbody>
  </table>

  <table width="100%" border="0" cellpadding="0" cellspacing="0">
    <tbody>
      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="1"></td>
      </tr>

      <tr>
        <td width="1" bgcolor="#CBDCED"><img alt="" width="2" height="1"></td>

        <td valign="top" width="175" nowrap="nowrap">
          &nbsp;<br>
          &nbsp;

          <h2>Google Instant</h2>
        </td>

        <td>&nbsp;<br>
        <font face="arial,sans-serif" size="-1"><input id="suggon" name="suggon"
        value="1" checked="checked" type="radio"><label for="suggon">Use Google
        Instant predictions and results appear while typing</label><br>
        <input id="suggmid" name="suggon" value="2" type="radio">
        <label for="suggmid">Do not use Google Instant</label><br>
        <br>
        Signed-in users can remove personalized predictions from their
        <a href="/history">Web History</a>. <a href="http://www.google.com/">
        Learn more</a><br>
        <br>
        &nbsp;</font></td>
      </tr>

      <tr>
        <td colspan="4" bgcolor="#CBDCED"><img alt="" width="1" height="2"></td>
      </tr>
    </tbody>
  </table><br>
  */});
  var submitButton = document.getElementsByName('submit2')[0];
  assertEquals(true, cvox.DomUtil.isControl(submitButton));
  var selectControl = document.getElementsByName('hl')[0];
  assertEquals(true, cvox.DomUtil.isControl(selectControl));
  var checkbox = $('paf');
  assertEquals(true, cvox.DomUtil.isControl(checkbox));
  var textInput = document.getElementsByName('luul')[0];
  assertEquals(true, cvox.DomUtil.isControl(textInput));
  var radioButton = $('suggmid');
  assertEquals(true, cvox.DomUtil.isControl(radioButton));
  var h1Elem = document.getElementsByTagName('h1');
  assertEquals(false, cvox.DomUtil.isControl(h1Elem));
});

/** Test if something is an ARIA control. @export */
TEST_F('CvoxDomUtilUnitTest', 'IsAriaControl', function() {
  this.loadDoc(function() {/*!
  <li id="cb1" role="checkbox" tabindex="0" aria-checked="false"
  aria-describedby="cond desc1">
      Lettuce
  </li>
  <li id="larger1" role="button" tabindex="0" aria-pressed="false"
  aria-labelledby="larger_label">+</li>
  <li id="r1" role="radio" tabindex="-1" aria-checked="false">Thai</li>
  <li id="treeitem1" role="treeitem" tabindex="-1">Oranges</li>
  */});
  var checkbox = $('cb1');
  assertEquals(true, cvox.DomUtil.isControl(checkbox));
  var button = $('larger1');
  assertEquals(true, cvox.DomUtil.isControl(button));
  var radio = $('r1');
  assertEquals(true, cvox.DomUtil.isControl(radio));
  var treeitem = $('treeitem1');
  assertEquals(false, cvox.DomUtil.isControl(treeitem));
});

/** Test if something is an focusable. @export */
TEST_F('CvoxDomUtilUnitTest', 'IsFocusable', function() {
  this.loadDoc(function() {/*!
  <a id="focus_link" href="#">Link</a>
  <a id="focus_anchor">Unfocusable anchor</a>
  <input id="focus_input" value="Input" />
  <select id="focus_select"><option>Select</option></select>
  <button id="focus_button1">Button</button>
  <button id="focus_button2" tabindex="-1">Button 2</button>
  <button id="focus_button3" tabindex="0">Button 3</button>
  <button id="focus_button4" tabindex="1">Button 4</button>
  <div id="focus_div1">Div</div>
  <div id="focus_div2" tabindex="-1">Div 2</div>
  <div id="focus_div3" tabindex="0">Div 3</div>
  <div id="focus_div4" tabindex="1">Div 4</div>
  */});
  var node;
  node = $('focus_link');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_anchor');
  assertEquals(false, cvox.DomUtil.isFocusable(node));
  node = $('focus_input');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_select');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_button1');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_button2');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_button3');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_button4');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_div1');
  assertEquals(false, cvox.DomUtil.isFocusable(node));
  node = $('focus_div2');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_div3');
  assertEquals(true, cvox.DomUtil.isFocusable(node));
  node = $('focus_div4');
  assertEquals(true, cvox.DomUtil.isFocusable(node));

  // Test it with null.
  assertEquals(false, cvox.DomUtil.isFocusable(null));

  // Test it with something that's not an element.
  assertEquals(false, cvox.DomUtil.isFocusable(new Object()));

  // Test it with a Text node.
  node = $('focus_button1').firstChild;
  assertEquals(false, cvox.DomUtil.isFocusable(node));
});

/** Some additional tests for getName function. */
TEST_F('CvoxDomUtilUnitTest', 'GetName', function() {
  this.loadDoc(function() {/*!
  <span id="test-span" aria-labelledby="fake-id">Some text</span>
  <label id="label1">One</label>
  <label id="label3">Label</label>
  <div id="test-div" aria-labelledby="label1 label2 label3"></div>
  */});
  var node = $('test-span');
  // Makes sure we can deal with invalid ids in aria-labelledby.
  var text = cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(node));
  assertEquals('Some text', text);
  node = $('test-div');
  text = cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(node));
  assertEquals('One Label', cvox.DomUtil.getName(node));
});

/** Test for getLinkURL. */
TEST_F('CvoxDomUtilUnitTest', 'GetLinkURL', function() {
  this.loadDoc(function() {/*!
  <a id="l1" name="nohref">Anchor</a>
  <a id="l2" href="">Empty link</a>
  <a id="l3" href="#">Link to self</a>
  <a id="l4" href="http://google.com">Google</a>
  <span id="l5" role="link" onClick="javascript:alert('?')">Something</span>
  <div id="l6" role="link">Div with link role</a>
  */});
  var node = $('l1');
  assertEquals('', cvox.DomUtil.getLinkURL(node));
  node = $('l2');
  assertEquals('', cvox.DomUtil.getLinkURL(node));
  node = $('l3');
  assertEquals('Internal link', cvox.DomUtil.getLinkURL(node));
  node = $('l4');
  assertEquals('http://google.com', cvox.DomUtil.getLinkURL(node));
  node = $('l5');
  assertEquals('Unknown link', cvox.DomUtil.getLinkURL(node));
  node = $('l6');
  assertEquals('Unknown link', cvox.DomUtil.getLinkURL(node));
});

/** Test for isDisabled. */
TEST_F('CvoxDomUtilUnitTest', 'IsDisabled', function() {
  this.loadDoc(function() {/*!
  <input id="button1" type="button" value="Press me!"/>
  <input id="button2" type="button" value="Don't touch me!" disabled/>
  */});
  var node = $('button1');
  assertEquals(false, cvox.DomUtil.isDisabled(node));
  node = $('button2');
  assertEquals(true, cvox.DomUtil.isDisabled(node));
});

/** Test for a tree with aria-expanded attribute. */
TEST_F('CvoxDomUtilUnitTest', 'Tree', function() {
  this.loadDoc(function() {/*!
  <div id=":0" role="tree" aria-selected="false" aria-expanded="true"
    aria-level="0" aria-labelledby=":0.label" tabindex="0"
    aria-activedescendant=":1">
    <span id=":0.label">Countries</span>
      <div class="goog-tree-item" id=":1" role="treeitem" aria-selected="true"
          aria-expanded="false" aria-labelledby=":1.label" aria-level="1">
        <span id=":1.label">A</span>
      </div>
      <div class="goog-tree-item" id=":2" role="treeitem" aria-selected="false"
          aria-expanded="false" aria-labelledby=":2.label" aria-level="1">
        <span id=":2.label">B<span>
      </div>
      <div class="goog-tree-item" id=":3" role="treeitem" aria-selected="false"
          aria-expanded="true" aria-labelledby=":3.label" aria-level="1">
        <span id=":3.label">C</span>
        <div class="goog-tree-children" role="group">
          <div class="goog-tree-item" id=":3a" role="treeitem"
              aria-selected="false" aria-expanded="false"
              aria-labelledby=":3a.label" aria-level="2">
            <span id=":3a.label">Chile</span>
          </div>
          <div class="goog-tree-item" id=":3b" role="treeitem"
              aria-selected="false" aria-expanded="false"
              aria-labelledby=":3b.label" aria-level="2">
            <span id=":3b.label">China</span>
          </div>
          <div class="goog-tree-item" id=":3c" role="treeitem"
              aria-selected="false" aria-expanded="false"
              aria-labelledby=":3c.label" aria-level="2">
            <span id=":3c.label">Christmas Island</span>
          </div>
          <div class="goog-tree-item" id=":3d" role="treeitem"
              aria-selected="false" aria-expanded="false"
              aria-labelledby=":3d.label" aria-level="2">
            <span id=":3d.label">Cocos (Keeling) Islands</span>
          </div>
        </div>
      </div>
  </div>
  */});
  var node = $(':0');
  assertEquals('A Collapsed Selected 1 of 3',
      cvox.DomUtil.getControlValueAndStateString(node));
  node = $(':1');
  assertEquals('A Collapsed Selected 1 of 3',
      cvox.DomUtil.getControlValueAndStateString(node));
  node = $(':2');
  assertEquals('B Collapsed Not selected 2 of 3',
      cvox.DomUtil.getControlValueAndStateString(node));
  node = $(':3');
  assertEquals('C Expanded Not selected 3 of 3',
      cvox.DomUtil.getControlValueAndStateString(node));
    node = $(':3b');
  assertEquals('China Collapsed Not selected 2 of 4',
      cvox.DomUtil.getControlValueAndStateString(node));
});

/** Test for tables with different border specifications */
TEST_F('CvoxDomUtilUnitTest', 'TableBorders', function() {
  this.loadDoc(function() {/*!
  <table id=":0" border="1">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":1" border="0">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":2" border="0px">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":3" frame="box">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":4" frame="void">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":5" style="border-width: medium">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":6" style="border-width: medium; border-style: none">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":7" style="border-color: red">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":8" style="border-style: dotted; border-width: 0px">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":9" style="border-width: 0px">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":10" style="border: 0px">
    <tr>
      <td>A</td>
    </tr>
  </table>
  <table id=":11" style="border: 0">
    <tr>
      <td>A</td>
    </tr>
  </table>
  */});
  var node = $(':0');
  assertTrue(cvox.DomUtil.hasBorder(node));

  node = $(':1');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':2');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':3');
  assertTrue(cvox.DomUtil.hasBorder(node));

  node = $(':4');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':5');
  assertTrue(cvox.DomUtil.hasBorder(node));

  node = $(':6');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':7');
  assertTrue(cvox.DomUtil.hasBorder(node));

  node = $(':8');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':9');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':10');
  assertFalse(cvox.DomUtil.hasBorder(node));

  node = $(':11');
  assertFalse(cvox.DomUtil.hasBorder(node));
});

/** Tests for shallowChildlessClone */
TEST_F('CvoxDomUtilUnitTest', 'ShallowChildlessClone', function() {
  this.loadDoc(function() {/*!
    <div id='simple'>asdf</div>
    <div id='expectedSimpleClone'>asdf</div>
    <div id='oneLevel'><div>asdf</div></div>
    <div id='expectedOneLevelClone'><div></div></div>
    <div id='withAttrs'><div class="asdf">asdf</div></div>
    <div id='expectedWithAttrsClone'><div class="asdf"></div></div>
  */});

  var simple = $('simple').firstChild;
  var expectedSimpleClone = $('expectedSimpleClone').firstChild;
  var oneLevel = $('oneLevel').firstChild;
  var expectedOneLevelClone = $('expectedOneLevelClone').firstChild;
  var withAttrs = $('withAttrs').firstChild;
  var expectedWithAttrsClone = $('expectedWithAttrsClone').firstChild;

  var simpleClone = cvox.DomUtil.shallowChildlessClone(simple);
  this.assertEqualsAsText_(simpleClone, expectedSimpleClone);

  var oneLevelClone = cvox.DomUtil.shallowChildlessClone(oneLevel);
  this.assertEqualsAsText_(oneLevelClone, expectedOneLevelClone);

  var withAttrsClone = cvox.DomUtil.shallowChildlessClone(withAttrs);
  this.assertEqualsAsText_(withAttrsClone, expectedWithAttrsClone);
});

/** Tests for deepClone */
TEST_F('CvoxDomUtilUnitTest', 'DeepClone', function() {
  this.loadDoc(function() {/*!
    <div id='simple'>asdf</div>
  */});
  var simpleClone = cvox.DomUtil.deepClone($('simple'));
  this.assertEqualsAsText_(simpleClone, $('simple'));

  this.loadDoc(function() {/*!
    <div id="withAttrs" class="asdf">asdf</div>
  */});
  var withAttrsClone = cvox.DomUtil.deepClone($('withAttrs'));
  this.assertEqualsAsText_(withAttrsClone, $('withAttrs'));
});

/** Tests for findNode */
TEST_F('CvoxDomUtilUnitTest', 'FindNode', function() {
  this.loadDoc(function() {/*!
    <div id="root">
      <p id="a">a</p>
      <a href="#" id="b">b</a>
    </div>
  */});
  var f = cvox.DomUtil.findNode;
  var node = f($('root'), function(n) {return n.id == 'b';});
  assertEquals('b', node.id);
});

/** Tests for getState for a list */
TEST_F('CvoxDomUtilUnitTest', 'ListLength', function() {
  this.loadDoc(function() {/*!
    <ul id="ul1">
      <li>A
      <li>B
      <li>C
    </ul>
    <ul id="ul2">
      <li aria-setsize="10">A
      <li aria-setsize="10">B
      <li aria-setsize="10">C
    </ul>
  */});
  var ul1 = $('ul1');
  assertEquals('with 3 items',
      cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getState(ul1)));

  var ul2 = $('ul2');
  assertEquals('with 10 items',
      cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getState(ul2)));
});

/** Tests for hasLongDesc */
TEST_F('CvoxDomUtilUnitTest', 'HasLongDesc', function() {
  this.loadDoc(function() {/*!
    <img id="img0" longdesc="desc.html" src="img0.jpg"></img>
    <img id="img1" src="img1.jpg"></img>
  */});
  var img0 = $('img0');
  assertEquals(true, cvox.DomUtil.hasLongDesc(img0));

  var img1 = $('img1');
  assertEquals(false, cvox.DomUtil.hasLongDesc(img1));
});

/** Tests for various link leaf types. */
TEST_F('CvoxDomUtilUnitTest', 'LinkLeaf', function() {
  this.loadDoc(function() {/*!
    <a id='leaf' href='google.com'><strong>Click</strong><div>here</div></a>
    <a id='non-leaf' href='google.com'>Click <h2>here</h2></a>
  */});
  var leaf = $('leaf');
  var nonLeaf = $('non-leaf');
  assertTrue(cvox.DomUtil.isLeafNode(leaf));
  assertFalse(cvox.DomUtil.isLeafNode(nonLeaf));
});


/** Test the value and state of a multiple select. */
TEST_F('CvoxDomUtilUnitTest', 'MultipleSelectValue', function() {
  this.loadDoc(function() {/*!
    <select id='cars' multiple>
      <option value="volvo">Volvo</option>
      <option value="saab">Saab</option>
      <option value="opel" selected>Opel</option>
      <option value="audi" selected>Audi</option>
    </select>
  */});
  var cars = $('cars');
  assertEquals('Opel to Audi', cvox.DomUtil.getValue(cars));
  assertEquals('selected 2 items', cvox.DomUtil.getState(cars));
});


/**
 * Test correctness of elementToPoint.
 *
 * Absolute positioning of the container is used to avoid the window of the
 * browser being too small to contain the test elements.
 */
TEST_F('CvoxDomUtilUnitTest', 'ElementToPoint', function() {
  this.loadDoc(function() {/*!
    <div style="position: absolute; top: 0; left: 0">
      <a id='one' href='#a'>First</a>
      <p id='two'>Some text</p>
      <ul><li id='three'>LI</li><li>LI2</li></ul>
    </div>
  */});
  var one = $('one');
  var two = $('two');
  var three = $('three');

  var oneHitPoint = cvox.DomUtil.elementToPoint(one);
  var twoHitPoint = cvox.DomUtil.elementToPoint(two);
  var threeHitPoint = cvox.DomUtil.elementToPoint(three);

  assertEquals(one, document.elementFromPoint(oneHitPoint.x, oneHitPoint.y));
  assertEquals(two, document.elementFromPoint(twoHitPoint.x, twoHitPoint.y));
  assertEquals(three,
               document.elementFromPoint(threeHitPoint.x, threeHitPoint.y));
});

/** Tests we compute the correct name for hidden aria labelledby nodes. */
TEST_F('CvoxDomUtilUnitTest', 'HiddenAriaLabelledby', function() {
  this.loadDoc(function() {/*!
    <span id="acc_name" style="display: none">
      hello world!
    </span>
    <button id="button" aria-labelledby="acc_name">
  */});
  assertEquals('hello world!',
               cvox.DomUtil.getName($('button')));
});

/** Tests that we compute the correct state for accesskeys. */
TEST_F('CvoxDomUtilUnitTest', 'AccessKey', function() {
  this.loadDoc(function() {/*!
    <a id='accessKey' href="#f" title="Next page" accesskey="n">Next page</a>
  */});
  var a = $('accessKey');
  assertEquals('has access key, n', cvox.DomUtil.getState(a));
});


/** Tests that we compute the correct name for ordered listitems. */
TEST_F('CvoxDomUtilUnitTest', 'OrderedListitem', function() {
  this.loadDoc(function() {/*!
    <ol id="fruits_ol">
      <li id='ol_li1'>apple
      <li id='ol_li2'>orange
      <li id='ol_li3'>strawberry
      <li id='ol_li4'>banana
    </ol>
  */});
  var li1 = $('ol_li1');
  var li2 = $('ol_li2');
  var li3 = $('ol_li3');
  var li4 = $('ol_li4');
  // Note that whitespace processing happens at a higher layer
  // (DescriptionUtil).
  assertEquals('1. apple',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li1)));
  assertEquals('2. orange',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li2)));
  assertEquals('3. strawberry',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li3)));
  assertEquals('4. banana',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li4)));

  $('fruits_ol').style.listStyleType = 'lower-latin';

  assertEquals('A. apple',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li1)));
  assertEquals('B. orange',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li2)));
  assertEquals('C. strawberry',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li3)));
  assertEquals('D. banana',
               cvox.DomUtil.collapseWhitespace(cvox.DomUtil.getName(li4)));
});

/** Tests a node with title, and textContent containing only whitespace. */
TEST_F('CvoxDomUtilUnitTest', 'TitleOverridesInnerWhitespace', function() {
  this.loadDoc(function() {/*!
    <button id="btn1" title="Remove from Chrome">
      <span class="lid"></span>
      <span class="can"></span>
    </button>
  */});
  var btn1 = $('btn1');
  assertEquals('Remove from Chrome', cvox.DomUtil.getName(btn1));
});

/** Test memoization. **/
TEST_F('CvoxDomUtilUnitTest', 'Memoization', function() {
  this.loadDoc(function() {/*!
    <div id="container">
    </div>
  */});

  // Nest divs 100 levels deep.
  var container = $('container');
  var outer = container;
  for (var i = 0; i < 100; i++) {
    var inner = document.createElement('div');
    outer.appendChild(inner);
    outer = inner;
  }
  var target = document.createElement('p');
  target.innerHTML = 'Text';
  outer.appendChild(target);

  var iterations = 200;

  function logTime(msg, fn) {
    var t0 = new Date();
    fn();
    console.log(msg + ' elapsed time: ' + (new Date() - t0) + ' ms');
  }

  // First, test without memoization.
  logTime('No memoization', function() {
    container.style.visibility = 'hidden';
    for (var i = 0; i < iterations; i++) {
      assertFalse(cvox.DomUtil.isVisible(target));
    }
    container.style.visibility = 'visible';
    for (var i = 0; i < iterations; i++) {
      assertTrue(cvox.DomUtil.isVisible(target));
    }
  });

  // Now test with memoization enabled.
  logTime('With memoization', function() {
    cvox.Memoize.scope(function() {
      container.style.visibility = 'hidden';
      for (var i = 0; i < iterations; i++) {
        assertFalse(cvox.DomUtil.isVisible(target));
      }
    });
    cvox.Memoize.scope(function() {
      container.style.visibility = 'visible';
      for (var i = 0; i < iterations; i++) {
        assertTrue(cvox.DomUtil.isVisible(target));
      }
    });
  });

  // Finally as a sanity check that things are being memoized, turn on
  // memoization and show that we get the wrong result if we change the
  // DOM and call isVisible again.
  cvox.Memoize.scope(function() {
    container.style.visibility = 'hidden';
    assertFalse(cvox.DomUtil.isVisible(target));

    container.style.visibility = 'visible';
    // This should be true! It will return the wrong answer because
    // we're deliberately leaving memoization on while modifying the DOM.
    assertFalse(cvox.DomUtil.isVisible(target));
  });
});
