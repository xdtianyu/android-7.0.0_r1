#
# Copyright 2008 Google Inc. All Rights Reserved.

"""
The shard module contains the objects and methods used to
manage shards in Autotest.

The valid actions are:
create:      creates shard
remove:      deletes shard(s)
list:        lists shards with label

See topic_common.py for a High Level Design and Algorithm.
"""

import sys
from autotest_lib.cli import topic_common, action_common


class shard(topic_common.atest):
    """shard class
    atest shard [create|delete|list] <options>"""
    usage_action = '[create|delete|list]'
    topic = msg_topic = 'shard'
    msg_items = '<shards>'

    def __init__(self):
        """Add to the parser the options common to all the
        shard actions"""
        super(shard, self).__init__()

        self.topic_parse_info = topic_common.item_parse_info(
            attribute_name='shards',
            use_leftover=True)


    def get_items(self):
        return self.shards


class shard_help(shard):
    """Just here to get the atest logic working.
    Usage is set by its parent"""
    pass


class shard_list(action_common.atest_list, shard):
    """Class for running atest shard list"""

    def execute(self):
        filters = {}
        if self.shards:
            filters['hostname__in'] = self.shards
        return super(shard_list, self).execute(op='get_shards',
                                               filters=filters)


    def warn_if_label_assigned_to_multiple_shards(self, results):
        """Prints a warning if one label is assigned to multiple shards.

        This should never happen, but if it does, better be safe.

        @param results: Results as passed to output().
        """
        assigned_labels = set()
        for line in results:
            for label in line['labels']:
                if label in assigned_labels:
                    sys.stderr.write('WARNING: label %s is assigned to '
                                     'multiple shards.\n'
                                     'This will lead to unpredictable behavor '
                                     'in which hosts and jobs will be assigned '
                                     'to which shard.\n')
                assigned_labels.add(label)


    def output(self, results):
        self.warn_if_label_assigned_to_multiple_shards(results)
        super(shard_list, self).output(results, ['hostname', 'labels'])


class shard_create(action_common.atest_create, shard):
    """Class for running atest shard create -l <label> <shard>"""
    def __init__(self):
        super(shard_create, self).__init__()
        self.parser.add_option('-l', '--labels',
                               help=('Assign LABELs to the SHARD. All jobs that '
                                     'require one of the labels will be run on  '
                                     'the shard. List multiple labels separated '
                                     'by a comma.'),
                               type='string',
                               metavar='LABELS')


    def parse(self):
        (options, leftover) = super(shard_create, self).parse(
                req_items='shards')
        if not options.labels:
            print ('Must provide one or more labels separated by a comma '
                  'with -l <labels>')
            self.parser.print_help()
            sys.exit(1)
        self.data_item_key = 'hostname'
        self.data['labels'] = options.labels
        return (options, leftover)


class shard_delete(action_common.atest_delete, shard):
    """Class for running atest shard delete <shards>"""

    def parse(self):
        (options, leftover) = super(shard_delete, self).parse()
        self.data_item_key = 'hostname'
        return (options, leftover)


    def execute(self, *args, **kwargs):
        print 'Please ensure the shard host is powered off.'
        print ('Otherwise DUTs might be used by multiple shards at the same '
               'time, which will lead to serious correctness problems.')
        return super(shard_delete, self).execute(*args, **kwargs)
