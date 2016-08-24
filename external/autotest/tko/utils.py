import os, sys, datetime, re


_debug_logger = sys.stderr
def dprint(msg):
    #pylint: disable-msg=C0111
    print >> _debug_logger, msg


def redirect_parser_debugging(ostream):
    #pylint: disable-msg=C0111
    global _debug_logger
    _debug_logger = ostream


def get_timestamp(mapping, field):
    #pylint: disable-msg=C0111
    val = mapping.get(field, None)
    if val is not None:
        val = datetime.datetime.fromtimestamp(int(val))
    return val


def find_toplevel_job_dir(start_dir):
    """ Starting from start_dir and moving upwards, find the top-level
    of the job results dir. We can't just assume that it corresponds to
    the actual job.dir, because job.dir may just be a subdir of the "real"
    job dir that autoserv was launched with. Returns None if it can't find
    a top-level dir.
    @param start_dir: starting directing for the upward search"""
    job_dir = start_dir
    while not os.path.exists(os.path.join(job_dir, ".autoserv_execute")):
        if job_dir == "/":
            return None
        job_dir = os.path.dirname(job_dir)
    return job_dir


def drop_redundant_messages(messages):
    """ Given a set of message strings discard any 'redundant' messages which
    are simple a substring of the existing ones.

    @param messages - a set of message strings

    @return - a subset of messages with unnecessary strings dropped
    """
    sorted_messages = sorted(messages, key=len, reverse=True)
    filtered_messages = set()
    for message in sorted_messages:
        for filtered_message in filtered_messages:
            if message in filtered_message:
                break
        else:
            filtered_messages.add(message)
    return filtered_messages


def get_afe_job_id(tag):
    """ Given a tag return the afe_job_id (if any).

    @param tag: afe_job_id is extracted from this tag

    @return: the afe_job_id as a string if regex matches, else return ''
    """
    return get_afe_job_id_and_hostname(tag)[0]


def get_afe_job_id_and_hostname(tag):
    """ Given a tag return the afe_job_id and hostname (if any).

    Extract job id and hostname if tag is in the format of
    JOB_ID-OWNER/HOSTNAME. JOB_ID and HOSTNAME must both be present
    to be considered as a match.

    @param tag: afe_job_id and hostname are extracted from this tag.
                e.g. "1234-chromeos-test/chromeos1-row1-host1"
    @return: A tuple (afe_job_id, hostname), both as string if regex
             matches, else return ('', '').
    """
    match = re.search('^([0-9]+)-.+/(.+)$', tag)
    return (match.group(1), match.group(2)) if match else ('', '')
