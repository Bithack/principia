#!/bin/env python3

from principia_base import principia_test
import re

def body_success(str):
    pattern = r'(\d+):(.*)'
    match = re.match(pattern, str)
    if match:
        version, message = match.groups()
        print('Version: {0}. Message: {1}'.format(version, message))
        return True

    print('Unable to verify body: "{0}"'.format(str))

    return False

def header_failure(headers):
    if 'HTTP/1.1 404 Not Found' in headers:
        return True

    return False

url = '/apZodIaL1/version_check.php'

for k, ua in principia_test.good_useragents.items():
    principia_test.run(url, useragent=ua, match_body=body_success)

for k, ua in principia_test.bad_useragents.items():
    principia_test.run(url, useragent=ua, match_headers=header_failure)
