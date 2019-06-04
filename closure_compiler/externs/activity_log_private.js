// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file was generated by:
//   tools/json_schema_compiler/compiler.py.
// NOTE: The format of types has changed. 'FooType' is now
//   'chrome.activityLogPrivate.FooType'.
// Please run the closure compiler before committing changes.
// See https://chromium.googlesource.com/chromium/src/+/master/docs/closure_compilation.md

/** @fileoverview Externs generated from namespace: activityLogPrivate */

/**
 * @const
 */
chrome.activityLogPrivate = {};

/**
 * @enum {string}
 */
chrome.activityLogPrivate.ExtensionActivityType = {
  API_CALL: 'api_call',
  API_EVENT: 'api_event',
  CONTENT_SCRIPT: 'content_script',
  DOM_ACCESS: 'dom_access',
  DOM_EVENT: 'dom_event',
  WEB_REQUEST: 'web_request',
};

/**
 * @enum {string}
 */
chrome.activityLogPrivate.ExtensionActivityFilter = {
  API_CALL: 'api_call',
  API_EVENT: 'api_event',
  CONTENT_SCRIPT: 'content_script',
  DOM_ACCESS: 'dom_access',
  DOM_EVENT: 'dom_event',
  WEB_REQUEST: 'web_request',
  ANY: 'any',
};

/**
 * @enum {string}
 */
chrome.activityLogPrivate.ExtensionActivityDomVerb = {
  GETTER: 'getter',
  SETTER: 'setter',
  METHOD: 'method',
  INSERTED: 'inserted',
  XHR: 'xhr',
  WEBREQUEST: 'webrequest',
  MODIFIED: 'modified',
};

/**
 * This corresponds to a row from the ActivityLog database. Fields will be blank if they were specified precisely in a lookup filter.
 * @typedef {{
 *   activityId: (string|undefined),
 *   extensionId: (string|undefined),
 *   activityType: !chrome.activityLogPrivate.ExtensionActivityType,
 *   time: (number|undefined),
 *   apiCall: (string|undefined),
 *   args: (string|undefined),
 *   count: (number|undefined),
 *   pageUrl: (string|undefined),
 *   pageTitle: (string|undefined),
 *   argUrl: (string|undefined),
 *   other: ({
 *     prerender: (boolean|undefined),
 *     domVerb: (!chrome.activityLogPrivate.ExtensionActivityDomVerb|undefined),
 *     webRequest: (string|undefined),
 *     extra: (string|undefined)
 *   }|undefined)
 * }}
 */
chrome.activityLogPrivate.ExtensionActivity;

/**
 * Used to specify values for a lookup.
 * @typedef {{
 *   extensionId: (string|undefined),
 *   activityType: !chrome.activityLogPrivate.ExtensionActivityFilter,
 *   apiCall: (string|undefined),
 *   pageUrl: (string|undefined),
 *   argUrl: (string|undefined),
 *   daysAgo: (number|undefined)
 * }}
 */
chrome.activityLogPrivate.Filter;

/**
 * This holds the results of a lookup, the filter of the lookup, the time of the lookup, and whether there are more results that match.
 * @typedef {{
 *   activities: !Array<!chrome.activityLogPrivate.ExtensionActivity>
 * }}
 */
chrome.activityLogPrivate.ActivityResultSet;

/**
 * Retrieves activity from the ActivityLog that matches the specified filter.
 * @param {!chrome.activityLogPrivate.Filter} filter Fill out the fields that
 *     you want to search for in the database.
 * @param {function(!chrome.activityLogPrivate.ActivityResultSet):void} callback
 */
chrome.activityLogPrivate.getExtensionActivities = function(filter, callback) {};

/**
 * Deletes activities in the ActivityLog database specified in the array of
 * activity IDs.
 * @param {!Array<string>} activityIds Erases only the activities which IDs are
 *     listed in the array.
 */
chrome.activityLogPrivate.deleteActivities = function(activityIds) {};

/**
 * Deletes the entire ActivityLog database.
 */
chrome.activityLogPrivate.deleteDatabase = function() {};

/**
 * Delete URLs in the ActivityLog database.
 * @param {!Array<string>=} urls Erases only the URLs listed; if empty, erases
 *     all URLs.
 */
chrome.activityLogPrivate.deleteUrls = function(urls) {};

/**
 * Fired when a given extension performs another activity.
 * @type {!ChromeEvent}
 */
chrome.activityLogPrivate.onExtensionActivity;
