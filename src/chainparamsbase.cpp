// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"
#include "util.h"
#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//
class CBaseMainParams : public CBaseChainParams {
public:
    CBaseMainParams() {
        networkID = CBaseChainParams::MAIN;
        nRPCPort = 31500;
    }
};

static CBaseMainParams mainParams;

//
// Testnet (v3)
//
class CBaseTestNetParams : public CBaseMainParams {
public:
    CBaseTestNetParams() {
        networkID = CBaseChainParams::TESTNET;
        nRPCPort = 31800;
        strDataDir = "testnet";
    }
};
static CBaseTestNetParams testNetParams;

static CBaseChainParams *pCurrentBaseParams = 0;

const CBaseChainParams &Params() {
    assert(pCurrentBaseParams);
    return *pCurrentBaseParams;
}

void SelectBaseParams(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            pCurrentBaseParams = &mainParams;
            break;
        case CBaseChainParams::TESTNET:
            pCurrentBaseParams = &testNetParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectBaseParamsFromCommandLine() {
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet) {
        return false;
    }

	if (fTestNet) {
        SelectBaseParams(CBaseChainParams::TESTNET);
    } else {
        SelectBaseParams(CBaseChainParams::MAIN);
    }
    return true;
} 