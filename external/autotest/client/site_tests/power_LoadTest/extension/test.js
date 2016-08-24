// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var cycle_tabs = {};
var cycles = {};
var time_ratio = 3600 * 1000 / test_time_ms; // default test time is 1 hour
var preexisting_windows = [];

function setupTest() {
  chrome.windows.getAll(null, function(windows) {
    preexisting_windows = windows;
    for (var i = 0; i < tasks.length; i++) {
      setTimeout(launch_task, tasks[i].start / time_ratio, tasks[i]);
    }
    var end = 3600 * 1000 / time_ratio
    setTimeout(send_status, end);
  });
}

function testListener(request, sender, sendResponse) {
  if (sender.tab.id in cycle_tabs) {
    cycle = cycle_tabs[sender.tab.id];
    cycle.successful_loads++;
    if (request.action == "should_scroll" && cycle.focus) {
      sendResponse({"should_scroll": should_scroll,
                    "should_scroll_up": should_scroll_up,
                    "scroll_loop": scroll_loop,
                    "scroll_interval": scroll_interval_ms,
                    "scroll_by": scroll_by_pixels});
    }
    delete cycle_tabs[sender.tab.id];
  }
}

function close_preexisting_windows() {
  for (var i = 0; i < preexisting_windows.length; i++) {
    chrome.windows.remove(preexisting_windows[i].id);
  }
  preexisting_windows.length = 0;
}

function cycle_navigate(cycle) {
  cycle_tabs[cycle.id] = cycle;
  var url = cycle.urls[cycle.idx];
  chrome.tabs.update(cycle.id, {'url': url, 'selected': true});
  cycle.idx = (cycle.idx + 1) % cycle.urls.length;
  if (cycle.timeout < cycle.delay / time_ratio && cycle.timeout > 0) {
    cycle.timer = setTimeout(cycle_check_timeout, cycle.timeout, cycle);
  } else {
    cycle.timer = setTimeout(cycle_navigate, cycle.delay / time_ratio, cycle);
  }
}

function cycle_check_timeout(cycle) {
  if (cycle.id in cycle_tabs) {
    cycle.failed_loads++;
    cycle_navigate(cycle);
  } else {
    cycle.timer = setTimeout(cycle_navigate,
                             cycle.delay / time_ratio - cycle.timeout,
                             cycle);
  }
}

function launch_task(task) {
  if (task.type == 'window' && task.tabs) {
    chrome.windows.create({'url': 'about:blank'}, function (win) {
      close_preexisting_windows();
      chrome.tabs.getSelected(win.id, function(tab) {
        chrome.tabs.update(tab.id, {'url': task.tabs[0], 'selected': true});
        for (var i = 1; i < task.tabs.length; i++) {
          chrome.tabs.create({'windowId': win.id, url: task.tabs[i]});
        }
        setTimeout(chrome.windows.remove, task.duration / time_ratio, win.id);
      });
    });
  } else if (task.type == 'cycle' && task.urls) {
    chrome.windows.create({'url': 'about:blank'}, function (win) {
      close_preexisting_windows();
      chrome.tabs.getSelected(win.id, function(tab) {
        var cycle = {
           'timeout': task.timeout,
           'name': task.name,
           'delay': task.delay,
           'urls': task.urls,
           'id': tab.id,
           'idx': 0,
           'timer': null,
           'focus': !!task.focus,
           'successful_loads': 0,
           'failed_loads': 0
        };
        cycles[task.name] = cycle;
        cycle_navigate(cycle);
        setTimeout(function(cycle, win_id) {
          clearTimeout(cycle.timer);
          chrome.windows.remove(win_id);
        }, task.duration / time_ratio, cycle, win.id);
      });
    });
  }
}

function send_status() {
  var post = ["status=good"];

  for (var name in cycles) {
    var cycle = cycles[name];
    post.push(name + "_successful_loads=" + cycle.successful_loads);
    post.push(name + "_failed_loads=" + cycle.failed_loads);
  }

  chrome.runtime.onMessage.removeListener(testListener);

  var log_url = 'http://localhost:8001/status';
  var req = new XMLHttpRequest();
  req.open('POST', log_url, true);
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  req.send(post.join("&"));
  console.log(post.join("&"));
}

function startTest() {
  time_ratio = 3600 * 1000 / test_time_ms; // default test time is 1 hour
  chrome.runtime.onMessage.addListener(testListener);
  setTimeout(setupTest, 1000);
}

function initialize() {
  // Called when the user clicks on the browser action.
  chrome.browserAction.onClicked.addListener(function(tab) {
    // Start the test with default settings.
    chrome.runtime.onMessage.addListener(testListener);
    setTimeout(setupTest, 1000);
  });
}

window.addEventListener("load", initialize);
