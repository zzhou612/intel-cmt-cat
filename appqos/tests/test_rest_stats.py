################################################################################
# BSD LICENSE
#
# Copyright(c) 2019-2022 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
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
################################################################################

"""
Unit tests for rest module STATS
"""

import json
from jsonschema import validate
import mock
import pytest

import common

from stats import StatsStore

from rest_common import get_config, load_json_schema, REST, CONFIG_EMPTY


class TestStats:
    @mock.patch("common.CONFIG_STORE.get_config", new=get_config)
    def test_get(self):
        def general_stats_get(get_stats_id):
            if get_stats_id == StatsStore.General.NUM_APPS_MOVES:
                return 1
            if get_stats_id == StatsStore.General.NUM_ERR:
                return 2

        with mock.patch("common.STATS_STORE.general_stats_get", side_effect=general_stats_get) as mock_func:
            response = REST.get("/stats")
            mock_func.assert_called()
        data = json.loads(response.data.decode('utf-8'))

        # validate response schema
        schema, resolver = load_json_schema('get_stats_response.json')
        validate(data, schema, resolver=resolver)

        assert response.status_code == 200
        assert data["num_apps_moves"] == 1
        assert data["num_err"] == 2
