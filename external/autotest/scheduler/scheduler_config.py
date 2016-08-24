import common
from autotest_lib.client.common_lib import global_config

CONFIG_SECTION = 'SCHEDULER'

class SchedulerConfig(object):
    """
    Contains configuration that can be changed during scheduler execution.
    """
    FIELDS = [
                ('max_processes_per_drone', int),
                ('max_processes_warning_threshold', float),
                ('clean_interval_minutes', int),
                ('max_parse_processes', int),
                ('tick_pause_sec', float),
                ('max_transfer_processes', int),
                ('secs_to_wait_for_atomic_group_hosts', int),
                ('reverify_period_minutes', int),
                ('reverify_max_hosts_at_once', int),
                ('max_repair_limit', int),
                ('max_provision_retries', int),
             ]


    def __init__(self):
        self.read_config()


    def read_config(self):
        """
        Reads the attributes (listed in `FIELDS`) from the global config
        and copies them into self.
        """
        config = global_config.global_config
        config.parse_config_file()
        for field, data_type in self.FIELDS:
            setattr(self, field, config.get_config_value(CONFIG_SECTION,
                                                         field,
                                                         type=data_type))


config = SchedulerConfig()
