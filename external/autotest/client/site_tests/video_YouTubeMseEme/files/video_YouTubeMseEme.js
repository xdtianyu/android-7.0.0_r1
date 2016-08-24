// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
"use strict";

(function() {
  window.__eventReporter = {};
  window.__testState = {};

  var logger = new Logger();
  window.__logger = logger;

  var video_format = 'video/mp4; codecs="avc1.640028"';
  var audio_format = 'audio/mp4; codecs="mp4a.40.2"';

  var audio1MB = createAudioDef(
      'http://localhost:8000/files/car-audio-1MB-trunc.mp4', 1048576, 65.875);
  var video1MB = createVideoDef(
      'http://localhost:8000/files/test-video-1MB.mp4', 1031034, 1.04);

  // Utility functions.
  function Logger() {
    var logs = [];

    this.log = function(log_string) {
      logs.push(log_string);
    };

    this.toString = function() {
      var output = '';
      for (var i in logs)
        output += logs[i];

      return output;
    }
  }

  function createAudioDef(src, size, duration) {
    return {
      type: 'audio',
      format: audio_format,
      size: size,
      src: src,
      duration: duration,
      bps: Math.floor(size / duration)
    };
  }

  function createVideoDef(src, size, duration) {
    return {
      type: 'video',
      format: video_format,
      size: size,
      src: src,
      duration: duration,
      bps: Math.floor(size / duration)
    };
  }

  function createMediaSource() {
    if (typeof MediaSource !== 'undefined')
      return new MediaSource();
    else
      return new WebKitMediaSource();
  }

  function createVideo() {
    return document.createElement('video');
  }

  function setupVideoAndMs(onSourceopen) {
    var temp_video = createVideo();
    var ms = createMediaSource();
    ms.addEventListener('webkitsourceopen', onSourceopen);
    var ms_url = window.URL.createObjectURL(ms);
    temp_video.src = ms_url;
    return {
      'video': temp_video,
      'ms': ms
    };
  }

  function XHRWrapper(file, onLoad, onError, start, length) {
    self = this;

    self.file = file;
    self.onLoad = onLoad;
    self.onError = onError;
    self.start = start;
    self.length = length;

    this.getResponseData = function() {
      var result = new Uint8Array(this.xhr.response);
      if (start != null) {
        return result.subarray(start, start + length);
      }
      return result;
    };

    this.abort = function() {
      this.xhr.abort();
    };

    this.send = function() {
      this.xhr.send();
    };

    this.xhr = new XMLHttpRequest();
    this.xhr.open('GET', file, true);

    this.xhr.addEventListener('load', function(e) {
      self.onLoad(e);
    });

    this.xhr.addEventListener('error', function(e) {
      logger.log('XHR errored.');
      self.onError(e);
    });

    this.xhr.addEventListener('timeout', function(e) {
      logger.log('XHR timed out.');
      self.onError(e);
    });

    this.xhr.responseType = 'arraybuffer';
    if (length != null) {
      start = start || 0;
      this.xhr.setRequestHeader(
        'Range',
        'bytes=' + start + '-' + (start + length - 1)
      );
    }
  }

  function approxEq(a, b) {
    return Math.abs(a - b) < 0.5;
  };

  // MSE tests.
  window.__testAttach = function() {
    var ms = createMediaSource();
    ms.addEventListener('webkitsourceopen', function() {
      window.__eventReporter['sourceopen'] = true;
    });

    var video = document.getElementById('main_player');
    video.src = window.URL.createObjectURL(ms);
    video.load();
  };

  window.__testAddSourceBuffer = function() {
    var vm = setupVideoAndMs(function() {
      try {
        var return_value = true;
        return_value &= vm.ms.sourceBuffers.length === 0;
        vm.ms.addSourceBuffer(audio_format);
        return_value &= vm.ms.sourceBuffers.length === 1;
        vm.ms.addSourceBuffer(video_format);
        return_value &= vm.ms.sourceBuffers.length === 2;

        window.__testState['addSourceBuffer'] = !!return_value;
      }
      catch (e) {
        window.__testState['addSourceBuffer'] = false;
      }
    });
  };

  window.__testAddSupportedFormats = function() {
    var formats = [
      audio_format,
      video_format,
    ];

    var vm = setupVideoAndMs(function() {
      for (var i = 0; i < formats.length; ++i) {
        try {
          vm.ms.addSourceBuffer(formats[i]);
        } catch (e) {
          window.__testState['addSupportedFormats'] = false;
          return;
        }
      }
      window.__testState['addSupportedFormats'] = true;
    });
  };

  window.__testAddSourceBufferException = function() {
    var vm = setupVideoAndMs(function() {
      try {
        vm.ms.addSourceBuffer('^^^');
        window.__testState['addSourceBufferException'] = false;
        return;
      }
      catch (e) {
        if (e.code !== DOMException.NOT_SUPPORTED_ERR) {
          window.__testState['addSourceBufferException'] = false;
          return;
        }
      }

      try {
        var temp_media_source = new WebKitMediaSource();
        temp_media_source.addSourceBuffer(audio_format);
        window.__testState['addSourceBufferException'] = false;
        return;
      }
      catch (e) {
        if (e.code !== DOMException.INVALID_STATE_ERR) {
          window.__testState['addSourceBufferException'] = false;
          return;
        }
      }
      window.__testState['addSourceBufferException'] = true;
    });
  };

  window.__testInitialVideoState = function() {
    var temp_video = createVideo();

    var test_result = isNaN(temp_video.duration);
    test_result &= temp_video.videoWidth === 0;
    test_result &= temp_video.videoHeight === 0;
    test_result &= temp_video.readyState === HTMLMediaElement.HAVE_NOTHING;
    test_result &= temp_video.src === '';
    test_result &= temp_video.currentSrc === '';

    window.__testState['initialVideoState'] = !!test_result;
  };

  window.__testInitialMSState = function() {
    var vm = setupVideoAndMs(
      function() {
        var test_result = true;
        test_result = test_result && isNaN(vm.ms.duration);
        test_result = test_result && vm.ms.readyState === 'open';
        window.__testState['initialMSState'] = test_result;
    });
  };

  function appendTestTemplate(
    test_name, media, test_func, abort, start, length, offset) {
    var vm = setupVideoAndMs(function() {
      var sb = vm.ms.addSourceBuffer(media.format);
      var xhr = new XHRWrapper(
        media.src,
        function(e) {
          var response_data = xhr.getResponseData();

          if (offset != null)
            sb.timestampOffset = offset;

          sb.append(response_data);

          if (abort != null) {
            sb.abort();
            sb.append(response_data);
          }

          var test_result = test_func(sb, media);
          window.__testState[test_name] = test_result;
        },
        function(e) {
          window.__testState[test_name] = false;
        }, start, length);

      xhr.send();
    });
  }

  function appendInnerTest(sb, media) {
    return sb.buffered.length === 1 && sb.buffered.start(0) === 0 &&
      approxEq(sb.buffered.end(0), media.duration);
  }

  window.__testAppend_audio = function() {
    appendTestTemplate('append_audio', audio1MB, appendInnerTest);
  };

  window.__testAppend_video = function() {
    appendTestTemplate('append_video', video1MB, appendInnerTest);
  };

  function appendAbortInnerTest(sb, media) {
    return sb.buffered.length === 1 && sb.buffered.start(0) === 0 &&
      sb.buffered.end(0) > 0;
  }

  window.__testAppendAbort_audio = function() {
    appendTestTemplate(
      'appendAbort_audio', audio1MB, appendAbortInnerTest, true, 0, 200000);
  };

  window.__testAppendAbort_video = function() {
    appendTestTemplate(
      'appendAbort_video', video1MB, appendAbortInnerTest, true, 0, 200000);
  };

  var TIMESTAMP_BUFFERED_OFFSET = 5;
  function appendTimestampOffsetTest(sb, media) {

    return sb.buffered.length === 1 &&
      sb.buffered.start(0) === TIMESTAMP_BUFFERED_OFFSET &&
      approxEq(sb.buffered.end(0), media.duration + TIMESTAMP_BUFFERED_OFFSET);
  }

  window.__testAppendTimestampOffset_audio = function() {
    appendTestTemplate(
      'appendTimestampOffset_audio', audio1MB, appendTimestampOffsetTest,
      null, null, null, TIMESTAMP_BUFFERED_OFFSET);
  };

  window.__testAppendTimestampOffset_video = function() {
    appendTestTemplate(
      'appendTimestampOffset_video', video1MB, appendTimestampOffsetTest,
      null, null, null, TIMESTAMP_BUFFERED_OFFSET);
  };

  window.__testDuration = function() {
    var vm = setupVideoAndMs(
      function() {
        var DURATION_TIME = 10;
        vm.ms.duration = DURATION_TIME;
        window.setTimeout(function() {
            window.__testState['duration'] = vm.ms.duration === DURATION_TIME;
        }, 20);
    });
  };

  function testDurationAfterAppend(test_name, media) {
    var vm = setupVideoAndMs(function() {
      var sb = vm.ms.addSourceBuffer(media.format);

      function onDurationChange() {
        window.__testState[test_name] = approxEq(
          vm.ms.duration, sb.buffered.end(0));
      }

      var xhr = new XHRWrapper(media.src, function() {
          var response_data = xhr.getResponseData();
          sb.append(response_data);
          sb.abort();
          vm.ms.duration = sb.buffered.end(0) / 2;
          vm.video.addEventListener('durationchange', onDurationChange);
          sb.append(response_data);
        });

      xhr.send();
    });
  };

  window.__testDurationAfterAppend_audio = function() {
    testDurationAfterAppend('durationAfterAppend_audio', audio1MB);
  };

  window.__testDurationAfterAppend_video = function() {
    testDurationAfterAppend('durationAfterAppend_video', video1MB);
  };

  window.__testSourceRemove = function() {
    var vm = setupVideoAndMs(
      function() {
        var sbAudio = vm.ms.addSourceBuffer(audio_format);
        var result = vm.ms.sourceBuffers.length === 1;
        vm.ms.removeSourceBuffer(sbAudio);
        result &= vm.ms.sourceBuffers.length === 0;

        sbAudio = vm.ms.addSourceBuffer(audio_format);
        result &= vm.ms.sourceBuffers.length === 1;
        for (var i = 0; i < 10; ++i) {
          var sbVideo = vm.ms.addSourceBuffer(video_format);
          result &= vm.ms.sourceBuffers.length === 2;
          vm.ms.removeSourceBuffer(sbVideo);
          result &= vm.ms.sourceBuffers.length === 1;
        }

        vm.ms.removeSourceBuffer(sbAudio);
        result &= vm.ms.sourceBuffers.length === 0;

        window.__testState['sourceRemove'] = !!result;
    });
  };

  // EME tests.
  window.__testCanPlayWebM = function() {
    var tempVideo = createVideo();
    return tempVideo.canPlayType(
        'video/webm; codecs="vp8,vorbis"') === 'probably' &&
      tempVideo.canPlayType(
        'audio/webm; codecs="vorbis"') === 'probably';
  };

  window.__testCanPlayClearKey = function() {
    var tempVideo = createVideo();
    return tempVideo.canPlayType(
        'video/mp4; codecs="avc1.640028"',
        'webkit-org.w3.clearkey') === 'probably' &&
      tempVideo.canPlayType(
        'audio/mp4; codecs="mp4a.40.2"',
        'webkit-org.w3.clearkey') === 'probably';
  };

  window.__testCanNotPlayPlayReady = function() {
    var tempVideo = createVideo();
    return tempVideo.canPlayType(
        'video/mp4; codecs="avc1.640028"',
        'com.youtube.playready') !== 'probably' &&
      tempVideo.canPlayType(
        'audio/mp4; codecs="mp4a.40.2"',
        'com.youtube.playready') !== 'probably';
  };

  window.__testCanPlayWidevine = function() {
    function createWidevineTest(mediaType) {
      var tempVideo = createVideo();

      return function(codecs, keySystem, criteria) {
        var codecString = mediaType;
        if (codecs != null)
          codecString += '; codecs="' + codecs + '"';

        var testResult = tempVideo.canPlayType(codecString, keySystem);

        if (criteria === null)
          return testResult === 'probably' || testResult === 'maybe';
        else if (typeof(criteria) === 'string')
          return testResult === criteria;
        else if (criteria.length) {
          var checks = false;
          for (var i in criteria)
            checks |= testResult === criteria[i];
          return !!checks;
        }
        return false;
      }
    }

    var audioTest = createWidevineTest('audio/webm');
    var videoTest = createWidevineTest('video/webm');

    var result = true;

    // Supported video formats.
    result &= videoTest(null, 'com.widevine.alpha', 'maybe');
    result &= videoTest(null, 'com.widevine', 'maybe');
    result &= videoTest('vp8', 'com.widevine.alpha', 'probably');
    result &= videoTest('vp8', 'com.widevine', 'probably');
    result &= videoTest('vp8.0', 'com.widevine.alpha', 'probably');
    result &= videoTest('vp8.0', 'com.widevine', 'probably');
    result &= videoTest('vorbis', 'com.widevine.alpha', 'probably');
    result &= videoTest('vorbis', 'com.widevine', 'probably');
    result &= videoTest('vp8,vp8.0,vorbis', 'com.widevine.alpha', 'probably');
    result &= videoTest('vp8,vp8.0,vorbis', 'com.widevine', 'probably');

    // Supported audio formats.
    result &= audioTest(null, 'com.widevine.alpha', 'maybe');
    result &= audioTest(null, 'com.widevine', 'maybe');
    result &= audioTest('vorbis', 'com.widevine.alpha', 'probably');
    result &= audioTest('vorbis', 'com.widevine', 'probably');

    // Unsupported video formats.
    result &= videoTest('codecs="vp8"', 'com.widevine.', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.foo', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.alpha.', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.alpha.foo', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.alph', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.alphb', '');
    result &= videoTest('codecs="vp8"', 'com.widevine.alphaa', '');
    result &= videoTest('codecs="avc1.640028"', 'com.widevine.alpha', '');
    result &= videoTest('codecs="mp4a"', 'com.widevine.alpha', '');

    // Unsported audio formats.
    result &= audioTest('codecs="vp8"', 'com.widevine', '');
    result &= audioTest('codecs="vp8,vorbis"', 'com.widevine.alpha', '');
    result &= audioTest('codecs="vp8,vorbis"', 'com.widevine.alpha', '');

    return result;
  };
})();
