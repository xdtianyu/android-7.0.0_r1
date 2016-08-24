#!/usr/bin/python

import unittest

import elasticsearch_mock as elasticsearch

class statsd_mock_test(unittest.TestCase):
    """Test statsd_mock"""
    def setUp(self):
        self.es = elasticsearch.Elasticsearch(host='host',
                                              port=1,
                                              timeout=10)


    def test_index_call_mock(self):
        """Test mock Elasticsearch.index method"""
        self.es.index(index='blah', doc_type='blah blah', body='random')


    def test_index_exists_mock(self):
        """Test mock Elasticsearch.indices.exists method"""
        self.es.indices.exists(index='random index')


    def test_index_delete_mock(self):
        """Test mock Elasticsearch.indices.delete method"""
        self.es.indices.delete(index='random index')


    def test_search_mock(self):
        """Test mock Elasticsearch.search method"""
        self.es.search(index='index', body='query')


    def test_exception_mock(self):
        """Test mock elasticsearch.ElasticsearchException method"""
        try:
            raise elasticsearch.ElasticsearchException('error message')
        except elasticsearch.ElasticsearchException as e:
            pass


if __name__ == '__main__':
    unittest.main()