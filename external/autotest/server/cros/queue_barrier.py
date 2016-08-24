# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from multiprocessing import Queue, queues


class QueueBarrierTimeout(Exception):
    """QueueBarrier timeout exception."""


class QueueBarrier(object):
    """This class implements a simple barrier to synchronize processes. The
    barrier relies on the fact that there a single process "master" and |n|
    different "slaves" to make the implementation simpler. Also, given this
    hierarchy, the slaves and the master can exchange a token while passing
    through the barrier.

    The so called "master" shall call master_barrier() while the "slave" shall
    call the slave_barrier() method.

    If the same group of |n| slaves and the same master are participating in the
    barrier, it is totally safe to reuse the barrier several times with the same
    group of processes.
    """


    def __init__(self, n):
        """Initializes the barrier with |n| slave processes and a master.

        @param n: The number of slave processes."""
        self.n_ = n
        self.queue_master_ = Queue()
        self.queue_slave_ = Queue()


    def master_barrier(self, token=None, timeout=None):
        """Makes the master wait until all the "n" slaves have reached this
        point.

        @param token: A value passed to every slave.
        @param timeout: The timeout, in seconds, to wait for the slaves.
                A None value will block forever.

        Returns the list of received tokens from the slaves.
        """
        # Wait for all the slaves.
        result = []
        try:
            for _ in range(self.n_):
                result.append(self.queue_master_.get(timeout=timeout))
        except queues.Empty:
            # Timeout expired
            raise QueueBarrierTimeout()
        # Release all the blocked slaves.
        for _ in range(self.n_):
            self.queue_slave_.put(token)
        return result


    def slave_barrier(self, token=None, timeout=None):
        """Makes a slave wait until all the "n" slaves and the master have
        reached this point.

        @param token: A value passed to the master.
        @param timeout: The timeout, in seconds, to wait for the slaves.
                A None value will block forever.
        """
        self.queue_master_.put(token)
        try:
            return self.queue_slave_.get(timeout=timeout)
        except queues.Empty:
            # Timeout expired
            raise QueueBarrierTimeout()
