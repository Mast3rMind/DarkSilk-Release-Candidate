// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_DARKSILK_SCRIPT_SIGN
#define H_DARKSILK_SCRIPT_SIGN

#include "script/scriptutils.h"
#include <vector>

class CKeyStore;
class CScript;
class CTransaction;
struct CMutableTransaction;

using namespace std;

extern unsigned nMaxDatacarrierBytes;

bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet);
bool Solver(const CKeyStore& keystore, const CScript& scriptPubKey, uint256 hash, int nHashType,
                  CScript& scriptSigRet, txnouttype& whichTypeRet);

inline bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
inline bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
inline bool SignSignature(const CKeyStore& keystore, const CTransaction& txFrom, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
inline bool SignSignature(const CKeyStore& keystore, const CMutableTransaction& txFrom, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);

// Given two sets of signatures for scriptPubKey, possibly with OP_0 placeholders,
// combine them intelligently and return the result.
CScript CombineSignatures(CScript scriptPubKey, const CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);

#endif // H_DARKSILK_SCRIPT_SIGN
