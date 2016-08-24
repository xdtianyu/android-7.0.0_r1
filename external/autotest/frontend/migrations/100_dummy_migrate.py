# This database migration does nothing. It works as a replacement for a reverted
# migrate CL: https://chromium-review.googlesource.com/#/c/253760
# For routine database migration revert, a new CL should be added to add
# DOWN_SQL of the reverted migration as UP_SQL. However, in that CL's case,
# DOWN_SQL is not available.
# The dummy migration avoid the requirement to manually downgrade migrate_info
# in each database.

UP_SQL = """
"""

DOWN_SQL="""
"""

