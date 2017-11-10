// Copyright (c) 2014-2017, The Superior Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  Parts of this file are originally copyright (c) 2013-2017 The Monero Project
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developersParts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

// FIXME: move this into a full wallet2 unit test suite, if possible

#include "gtest/gtest.h"

#include "wallet/wallet2.h"
#include "common/dns_utils.h"
#include <string>

TEST(AddressFromTXT, Success)
{
  std::string addr = "﻿5QaiHzo64sLDo42ky98uXtJ3zswCdpUrk1q5nSidtqovjjiC7FmxRt84Zu3HkpYQX1PLDU72aQMK6Cif4muRxwt3RyZXY6y";

  std::string txtr = "oa1:sup";
  txtr += " recipient_address=";
  txtr += addr;
  txtr += ";";

  std::string res = tools::dns_utils::address_from_txt_record(txtr);

  EXPECT_STREQ(addr.c_str(), res.c_str());

  std::string txtr2 = "foobar";

  txtr2 += txtr;

  txtr2 += "more foobar";

  res = tools::dns_utils::address_from_txt_record(txtr2);

  EXPECT_STREQ(addr.c_str(), res.c_str());

  std::string txtr3 = "foobar oa1:sup tx_description=\"Donation for Superior Development Fund\"; ";
  txtr3 += "recipient_address=";
  txtr3 += addr;
  txtr3 += "; foobar";

  res = tools::dns_utils::address_from_txt_record(txtr3);

  EXPECT_STREQ(addr.c_str(), res.c_str());
}

TEST(AddressFromTXT, Failure)
{
  std::string txtr = "oa1:sup recipient_address=not a real address";

  std::string res = tools::dns_utils::address_from_txt_record(txtr);

  ASSERT_STREQ("", res.c_str());

  txtr += ";";

  res = tools::dns_utils::address_from_txt_record(txtr);
  ASSERT_STREQ("", res.c_str());
}

TEST(AddressFromURL, Success)
{
  const std::string addr = "﻿5QaiHzo64sLDo42ky98uXtJ3zswCdpUrk1q5nSidtqovjjiC7FmxRt84Zu3HkpYQX1PLDU72aQMK6Cif4muRxwt3RyZXY6y";
  
  bool dnssec_result = false;

  std::vector<std::string> addresses = tools::dns_utils::addresses_from_url("donate.superior-coin.com", dnssec_result);

  EXPECT_EQ(1, addresses.size());
  if (addresses.size() == 1)
  {
    EXPECT_STREQ(addr.c_str(), addresses[0].c_str());
  }

  // OpenAlias address with an @ instead of first .
  addresses = tools::dns_utils::addresses_from_url("donate@superior-coin.com", dnssec_result);
  EXPECT_EQ(1, addresses.size());
  if (addresses.size() == 1)
  {
    EXPECT_STREQ(addr.c_str(), addresses[0].c_str());
  }
}

TEST(AddressFromURL, Failure)
{
  bool dnssec_result = false;

  std::vector<std::string> addresses = tools::dns_utils::addresses_from_url("example.invalid", dnssec_result);

  // for a non-existing domain such as "example.invalid", the non-existence is proved with NSEC records
  ASSERT_TRUE(dnssec_result);

  ASSERT_EQ(0, addresses.size());
}
