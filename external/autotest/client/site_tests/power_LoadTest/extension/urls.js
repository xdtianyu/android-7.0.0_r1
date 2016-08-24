// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// List of tasks to accomplish
var URLS = new Array();

var ViewGDoc = ('https://docs.google.com/document/d/');

var tasks = [
  {
    // Chrome browser window 1. This window remains open for the entire test.
    type: 'window',
    name: 'background',
    start: 0,
    duration: minutes(60),
    focus: false,
    tabs: [
     'http://www.google.com',
     'http://news.google.com',
     'http://finance.yahoo.com',
     'http://clothing.shop.ebay.com/Womens-Shoes-/63889/i.html',
     'http://www.facebook.com'
    ]
  },
  {
    // Page cycle through popular external websites for 36 minutes
    type: 'cycle',
    name: 'web',
    start: seconds(1),
    duration: minutes(36),
    delay: seconds(60), // A minute on each page
    timeout: seconds(10),
    focus: true,
    urls: URLS,
  },
  {
    // After 36 minutes, actively read e-mail for 12 minutes
    type: 'cycle',
    name: 'email',
    start: minutes(36) + seconds(1),
    duration: minutes(12) - seconds(1),
    delay: minutes(5), // 5 minutes between full gmail refresh
    timeout: seconds(10),
    focus: true,
    urls: [
       'http://gmail.com',
       'http://mail.google.com'
    ],
  },
  {
    // After 36 minutes, start streaming audio (background tab), total playtime
    // 12 minutes
    type: 'cycle',
    name: 'audio',
    start: minutes(36),
    duration: minutes(12),
    delay: minutes(12),
    timeout: seconds(10),
    focus: false,
    urls: [
      'http://www.bbc.co.uk/worldservice/audioconsole/?stream=live',
      'http://www.npr.org/templates/player/mediaPlayer.html?action=3&t=live1',
      'http://www.cbc.ca/radio2/channels/popup.html?stream=classical'
    ]
  },
  {
    // After 48 minutes, play with Google Docs for 6 minutes
    type: 'cycle',
    name: 'docs',
    start: minutes(48),
    duration: minutes(6),
    delay: minutes(1), // A minute on each page
    timeout: seconds(10),
    focus: true,
    urls: [
       ViewGDoc + '1CIvneyASuIHvxxN0WV22zikb08Us1nc93mkU0c5Azr4/edit',
       ViewGDoc + '120TtfoHXCgRuaubGhra3X5tl0_pS7KX757wFigTFf0c/edit'
    ],
  },
  {
    // After 54 minutes, watch Big Buck Bunny for 6 minutes
    type: 'window',
    name: 'video',
    start: minutes(54),
    duration: minutes(6),
    focus: true,
    tabs: [
        'http://www.youtube.com/embed/YE7VzlLtp-4?start=236&vq=hd720&autoplay=1'
    ]
  },
];


// List of URLs to cycle through
var u_index = 0;
URLS[u_index++] = 'http://www.google.com';
URLS[u_index++] = 'http://www.yahoo.com';
URLS[u_index++] = 'http://www.facebook.com';
URLS[u_index++] = 'http://www.youtube.com';
URLS[u_index++] = 'http://www.wikipedia.org';
URLS[u_index++] = 'http://www.amazon.com';
URLS[u_index++] = 'http://www.msn.com';
URLS[u_index++] = 'http://www.bing.com';
URLS[u_index++] = 'http://www.blogspot.com';
URLS[u_index++] = 'http://www.microsoft.com';
URLS[u_index++] = 'http://www.myspace.com';
URLS[u_index++] = 'http://www.go.com';
URLS[u_index++] = 'http://www.walmart.com';
URLS[u_index++] = 'http://www.about.com';
URLS[u_index++] = 'http://www.target.com';
URLS[u_index++] = 'http://www.aol.com';
URLS[u_index++] = 'http://www.mapquest.com';
URLS[u_index++] = 'http://www.ask.com';
URLS[u_index++] = 'http://www.craigslist.org';
URLS[u_index++] = 'http://www.wordpress.com';
URLS[u_index++] = 'http://www.answers.com';
URLS[u_index++] = 'http://www.paypal.com';
URLS[u_index++] = 'http://www.imdb.com';
URLS[u_index++] = 'http://www.bestbuy.com';
URLS[u_index++] = 'http://www.ehow.com';
URLS[u_index++] = 'http://www.photobucket.com';
URLS[u_index++] = 'http://www.cnn.com';
URLS[u_index++] = 'http://www.chase.com';
URLS[u_index++] = 'http://www.att.com';
URLS[u_index++] = 'http://www.sears.com';
URLS[u_index++] = 'http://www.weather.com';
URLS[u_index++] = 'http://www.apple.com';
URLS[u_index++] = 'http://www.zynga.com';
URLS[u_index++] = 'http://www.adobe.com';
URLS[u_index++] = 'http://www.bankofamerica.com';
URLS[u_index++] = 'http://www.zedo.com';
URLS[u_index++] = 'http://www.flickr.com';
URLS[u_index++] = 'http://www.shoplocal.com';
URLS[u_index++] = 'http://www.twitter.com';
URLS[u_index++] = 'http://www.cnet.com';
URLS[u_index++] = 'http://www.verizonwireless.com';
URLS[u_index++] = 'http://www.kohls.com';
URLS[u_index++] = 'http://www.bizrate.com';
URLS[u_index++] = 'http://www.jcpenney.com';
URLS[u_index++] = 'http://www.netflix.com';
URLS[u_index++] = 'http://www.fastclick.net';
URLS[u_index++] = 'http://www.windows.com';
URLS[u_index++] = 'http://www.questionmarket.com';
URLS[u_index++] = 'http://www.nytimes.com';
URLS[u_index++] = 'http://www.toysrus.com';
URLS[u_index++] = 'http://www.allrecipes.com';
URLS[u_index++] = 'http://www.overstock.com';
URLS[u_index++] = 'http://www.comcast.net';

