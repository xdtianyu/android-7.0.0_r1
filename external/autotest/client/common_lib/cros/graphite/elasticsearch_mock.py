# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import stats_es_mock


class Elasticsearch(stats_es_mock.mock_class_base):
    """mock class for es_mock"""
    pass


class ElasticsearchException(Exception):
    """Mock class for elcasticsearch.ElasticsearchException"""
    pass