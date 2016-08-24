"""Modules under acts.controllers provide interfaces to hardware/software
resources that ACTS manages.

Top level controllers module are controller modules that need to be explicitly
specified by users in test configuration files. Top level controller modules
should have the following module level functions:

def create(configs, logger):
    '''Instantiates the controller class with the input configs.
    Args:
        configs: A list of dicts each representing config for one controller
            object.
        logger: The main logger used in the current test run.
    Returns:
        A list of controller objects.

def destroy(objs):
    '''Destroys a list of controller objects created by the "create" function
    and releases all the resources.

    Args:
        objs: A list of controller objects created from this module.
    '''
"""

"""This is a list of all the top level controller modules"""
__all__ = [
    "android_device",
    "attenuator",
    "monsoon",
    "access_point",
    "iperf_server"
]