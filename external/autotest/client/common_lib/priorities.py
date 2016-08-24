
from autotest_lib.client.common_lib import enum

# We include a 'Default' level just below what BVT will run at so that when
# the priority rework code is rolled out, any code that doesn't specify a
# priority, such as suites on old branches, will inherit a priority that makes
# them a best effort without lengthening important build processes.
Priority = enum.Enum('Weekly', 'Daily', 'PostBuild', 'Default', 'Build',
                     'PFQ', 'CQ', start_value=10, step=10)
