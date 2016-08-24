import logging, logging.handlers, time, os
import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.site_utils import rpc_logserver


config = global_config.global_config
LOGGING_ENABLED = config.get_config_value('SERVER', 'rpc_logging', type=bool)

rpc_logger = None


def configure_logging():
    logserver_enabled = config.get_config_value(
            'SERVER', 'rpc_logserver', type=bool)
    if logserver_enabled:
        handler = logging.handlers.SocketHandler(
                'localhost', rpc_logserver.DEFAULT_PORT)
    else:
        handler = rpc_logserver.get_logging_handler()

    global rpc_logger
    rpc_logger = logging.getLogger('rpc_logger')
    rpc_logger.addHandler(handler)
    rpc_logger.propagate = False
    rpc_logger.setLevel(logging.DEBUG)


if LOGGING_ENABLED:
    configure_logging()
