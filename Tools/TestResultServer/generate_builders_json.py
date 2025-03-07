#!/usr/bin/env python
# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import json
import logging
import optparse
import os
import urllib2

# FIXME: See if Tools/Scripts/webkitpy/layout_tests/port/builders.py should also read
# the output json file here as its data source.


def master_json_url(master_url):
    return master_url + '/json/builders'


def builder_json_url(master_url, builder):
    return master_json_url(master_url) + '/' + urllib2.quote(builder)


def cached_build_json_url(master_url, builder, build_number):
    return builder_json_url(master_url, builder) + '/builds/' + str(build_number)


def fetch_json(url):
    logging.debug('Fetching %s' % url)
    return json.load(urllib2.urlopen(url))


def insert_builder_and_test_data(masters):
    for master in masters:
        master_url = master['url']
        tests_object = {}
        master['tests'] = tests_object

        for builder in fetch_json(master_json_url(master_url)):
            build_data = fetch_json(builder_json_url(master_url, builder))
            cached_builds = build_data['cachedBuilds']
            current_builds = build_data['currentBuilds']

            latest_cached_build = cached_builds.pop()
            while latest_cached_build in current_builds and len(cached_builds):
                latest_cached_build = cached_builds.pop()

            for step in fetch_json(cached_build_json_url(master_url, builder, latest_cached_build))['steps']:
                step_name = step['name']

                # The chromium bots call this step webkit-tests, the webkit.org bots call it layout-test. :(
                # The files stored at webkit-test-results.appspot.com use layout-tests as the test suite name, so normalize to that.
                if step_name in ['layout-test', 'webkit_tests']:
                    step_name = 'layout-tests'

                is_test = step_name == 'layout-tests' if master['name'] == 'webkit.org' else 'test' in step_name and 'archive' not in step_name
                if not is_test:
                    continue

                if step_name not in tests_object:
                    tests_object[step_name] = {'builders': []}
                tests_object[step_name]['builders'].append(builder)

    for step_name in tests_object:
        tests_object[step_name]['builders'].sort()


def main():
    option_parser = optparse.OptionParser()
    option_parser.add_option('-v', '--verbose', action='store_true', default=False, help='Print debug logging')
    options, args = option_parser.parse_args()

    logging.getLogger().setLevel(logging.DEBUG if options.verbose else logging.INFO)

    masters = [
        {'name': 'ChromiumWin', 'url': 'http://build.chromium.org/p/chromium.win'},
        {'name': 'ChromiumMac', 'url': 'http://build.chromium.org/p/chromium.mac'},
        {'name': 'ChromiumLinux', 'url': 'http://build.chromium.org/p/chromium.linux'},
        {'name': 'ChromiumChromiumOS', 'url': 'http://build.chromium.org/p/chromium.chromiumos'},
        {'name': 'ChromiumGPU', 'url': 'http://build.chromium.org/p/chromium.gpu'},
        {'name': 'ChromiumGPUFYI', 'url': 'http://build.chromium.org/p/chromium.gpu.fyi'},
        {'name': 'ChromiumPerfAv', 'url': 'http://build.chromium.org/p/chromium.perf_av'},
        {'name': 'ChromiumWebkit', 'url': 'http://build.chromium.org/p/chromium.webkit'},
        {'name': 'ChromiumFYI', 'url': 'http://build.chromium.org/p/chromium.fyi'},
        {'name': 'webkit.org', 'url': 'http://build.webkit.org'},
    ]

    insert_builder_and_test_data(masters)

    json_file_prefix = ('// This file is auto-generated by Tools/TestResultServer/generate_builders_json.py. It should not be manually modified.\n'
        '// It uses jsonp instead of proper json because we want to be able to load it from a file URL in Chrome for local testing.\n'
        'LOAD_BUILDBOT_DATA(')
    json_file_suffix = ');\n';

    json_file = open(os.path.join('static-dashboards', 'builders.jsonp'), 'w')
    json_file.write(json_file_prefix + json.dumps(masters, separators=(', ', ': '), indent=4, sort_keys=True) + json_file_suffix)


if __name__ == "__main__":
    main()
