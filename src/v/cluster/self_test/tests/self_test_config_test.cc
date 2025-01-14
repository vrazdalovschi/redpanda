// Copyright 2023 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#define BOOST_TEST_MODULE self_test

#include "cluster/self_test/diskcheck.h"
#include "cluster/self_test/netcheck.h"
#include "json/document.h"

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_diskcheck_validation) {
    namespace cft = cluster::self_test;

    BOOST_CHECK_THROW(
      cft::diskcheck::validate_options(
        cluster::diskcheck_opts{.skip_write = true, .skip_read = true}),
      cft::diskcheck_option_out_of_range);

    BOOST_CHECK_THROW(
      cft::diskcheck::validate_options(
        cluster::diskcheck_opts{.duration = 100ms}),
      cft::diskcheck_option_out_of_range);
    BOOST_CHECK_THROW(
      cft::diskcheck::validate_options(
        cluster::diskcheck_opts{.duration = 10min}),
      cft::diskcheck_option_out_of_range);

    BOOST_CHECK_THROW(
      cft::diskcheck::validate_options(
        cluster::diskcheck_opts{.parallelism = 0}),
      cft::diskcheck_option_out_of_range);
    BOOST_CHECK_THROW(
      cft::diskcheck::validate_options(
        cluster::diskcheck_opts{.parallelism = 266}),
      cft::diskcheck_option_out_of_range);

    BOOST_CHECK_NO_THROW(
      cft::diskcheck::validate_options(cluster::diskcheck_opts{
        .skip_write = true,
        .skip_read = false,
        .duration = 5000ms,
        .parallelism = 50}));
}

BOOST_AUTO_TEST_CASE(test_netcheck_validation) {
    namespace cft = cluster::self_test;
    std::vector<model::node_id> peers{model::node_id(0)};

    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(cluster::netcheck_opts{}),
      cft::netcheck_exception);

    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .request_size = 0}),
      cft::netcheck_option_out_of_range);
    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .request_size = (1ULL << 28)}),
      cft::netcheck_option_out_of_range);

    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .parallelism = 0}),
      cft::netcheck_option_out_of_range);
    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .parallelism = 266}),
      cft::netcheck_option_out_of_range);

    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .duration = 100ms}),
      cft::netcheck_option_out_of_range);
    BOOST_CHECK_THROW(
      cft::netcheck::validate_options(
        cluster::netcheck_opts{.peers = peers, .duration = 50min}),
      cft::netcheck_option_out_of_range);

    BOOST_CHECK_NO_THROW(cft::netcheck::validate_options(cluster::netcheck_opts{
      .peers = peers,
      .request_size = 1500,
      .duration = 5000ms,
      .parallelism = 25}));
}

static const std::string sample_self_test_config = R"(
{
    "tests": [
        {
            "name" : "my disk test",
            "dsync" : false,
            "skip_write" : true,
            "skip_read" : false,
            "data_size" : 500000,
            "request_size" : 330000,
            "duration_ms" : 10000,
            "parallelism" : 50,
            "type" : "disk"
        },
        {
            "name": "my network test",
            "request_size": 54321,
            "duration_ms": 7100,
            "parallelism": 25,
            "type" : "network"
        }
    ]
}
)";

BOOST_AUTO_TEST_CASE(test_self_test_json_serde) {
    json::Document doc;
    doc.Parse(sample_self_test_config);
    BOOST_TEST(
      !doc.Parse(sample_self_test_config).HasParseError(), "Invalid JSON");
    BOOST_TEST(doc.HasMember("tests"), "Invalid JSON");
    BOOST_TEST(doc["tests"].IsArray(), "Invalid JSON");
    const auto& arr = doc["tests"].GetArray();
    BOOST_TEST(arr.Size() == 2, "Invalid JSON");
    BOOST_TEST(arr[0].IsObject(), "Invalid JSON");
    BOOST_TEST(arr[1].IsObject(), "Invalid JSON");

    const auto& dsk_json_obj = arr[0].GetObject();
    const auto& net_json_obj = arr[1].GetObject();
    BOOST_TEST(
      (dsk_json_obj.HasMember("type")
       && ss::sstring(dsk_json_obj["type"].GetString()) == "disk"),
      "Invalid JSON");
    BOOST_TEST(
      (net_json_obj.HasMember("type")
       && ss::sstring(net_json_obj["type"].GetString()) == "network"),
      "Invalid JSON");

    auto dsk_opts = cluster::diskcheck_opts::from_json(dsk_json_obj);
    BOOST_CHECK_EQUAL(dsk_opts.name, "my disk test");
    BOOST_CHECK_EQUAL(dsk_opts.dsync, false);
    BOOST_CHECK_EQUAL(dsk_opts.skip_write, true);
    BOOST_CHECK_EQUAL(dsk_opts.skip_read, false);
    BOOST_CHECK_EQUAL(dsk_opts.data_size, 500000);
    BOOST_CHECK_EQUAL(dsk_opts.request_size, 330000);
    BOOST_CHECK_EQUAL(dsk_opts.duration, 10000ms);
    BOOST_CHECK_EQUAL(dsk_opts.parallelism, 50);

    auto net_opts = cluster::diskcheck_opts::from_json(net_json_obj);
    BOOST_CHECK_EQUAL(net_opts.name, "my network test");
    BOOST_CHECK_EQUAL(net_opts.request_size, 54321);
    BOOST_CHECK_EQUAL(net_opts.duration, 7100ms);
    BOOST_CHECK_EQUAL(net_opts.parallelism, 25);
}
