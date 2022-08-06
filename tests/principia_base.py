class principia_test:
    # expected_header should be an array of headers we're interested in
    # expected_output should be a functino which matches the output via
    # regex or any other method that is requires

    base_url = "http://principiagame.com"

    good_useragents = {
            'android' : 'Principia/26 (Android)',
            'ios'     : 'Principia/26 (iOS)',
            'linux'   : 'Principia/26 (Linux)',
            'windows' : 'Principia/26 (Windows)',
            'unknown' : 'Principia/26 (Testsuite)',
            }

    bad_useragents = {
            'bad'     : 'Semoropi Dalodi Testsuite',
            }

    def run(url, useragent='Principia/26 (Android)', match_headers=None, match_body=None):
        import sys
        import pycurl
        import io

        wheader = io.BytesIO();
        wbody   = io.BytesIO();

        curl = pycurl.Curl()

        curl.setopt(pycurl.URL, principia_test.base_url + url)
        curl.setopt(pycurl.USERAGENT, useragent)
        curl.setopt(pycurl.HEADERFUNCTION, wheader.write)
        curl.setopt(pycurl.WRITEFUNCTION,  wbody.write)

        curl.perform()
        curl.close()

        headers = wheader.getvalue().decode(encoding='UTF-8').split("\r\n")
        body = wbody.getvalue().decode(encoding='UTF-8')

        if match_headers:
            assert(match_headers(headers) is True)

        if match_body:
            assert(match_body(body) is True)

        return True
