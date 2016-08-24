import common
import MySQLdb as driver
import db
from autotest_lib.client.common_lib.cros import retry

class db_mysql(db.db_sql):
    @retry.retry(db._get_error_class("OperationalError"), timeout_min=2,
                 delay_sec=0.5)
    def connect(self, host, database, user, password, port):
        connection_args = {
            'host': host,
            'user': user,
            'db': database,
            'passwd': password,
        }
        if port:
            connection_args['port'] = int(port)
        return driver.connect(**connection_args)
