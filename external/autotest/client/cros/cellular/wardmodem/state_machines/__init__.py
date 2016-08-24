import os
# Add all python modules to __all__, skipping unittests
__all__ = []
for file_name in os.listdir(os.path.dirname(__file__)):
    if (file_name.endswith('.py') and
        file_name not in ['common.py', '__init__.py'] and
        file_name.find('unittest') == -1):
        module_name = file_name.rstrip('.py')
        __all__.append(module_name)
print 'State machines to load: %s' % str(__all__)
