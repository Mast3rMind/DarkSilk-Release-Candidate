// Copyright (c) 2009-2016 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Developers
// Copyright (c) 2015-2016 The Silk Network Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"

using namespace std;

CTxMemPool::CTxMemPool(const CFeeRate& _minRelayFee) :
    nTransactionsUpdated(0),
    minRelayFee(_minRelayFee)
{
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;

    // 25 blocks is a compromise between using a lot of disk/memory and
    // trying to give accurate estimates to people who might be willing
    // to wait a day or two to save a fraction of a penny in fees.
    // Confirmation times for very-low-fee transactions that take more
    // than an hour or three to confirm are highly variable.
    minerPolicyEstimator = new CMinerPolicyEstimator(25);
}

CTxMemPool::~CTxMemPool()
{
    delete minerPolicyEstimator;
}

unsigned int CTxMemPool::GetTransactionsUpdated() const
{
    LOCK(cs);
    return nTransactionsUpdated;
}

void CTxMemPool::AddTransactionsUpdated(unsigned int n)
{
    LOCK(cs);
    nTransactionsUpdated += n;
}

bool CTxMemPool::addUnchecked(const uint256& hash, CTransaction &tx)
{
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    LOCK(cs);
    {
        mapTx[hash] = tx;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            mapNextTx[tx.vin[i].prevout] = CInPoint(&mapTx[hash], i);
        nTransactionsUpdated++;
    }
    return true;
}

bool CTxMemPool::remove(const CTransaction &tx, bool fRecursive)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        uint256 hash = tx.GetHash();
        if (mapTx.count(hash))
        {
            if (fRecursive) {
                for (unsigned int i = 0; i < tx.vout.size(); i++) {
                    std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                    if (it != mapNextTx.end())
                        remove(*it->second.ptx, true);
                }
            }
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
                mapNextTx.erase(txin.prevout);
            mapTx.erase(hash);
            nTransactionsUpdated++;
        }
    }
    return true;
}

bool CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    LOCK(cs);
    BOOST_FOREACH(const CTxIn &txin, tx.vin) {
        std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
                remove(txConflict, true);
        }
    }
    return true;
}

void CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    ++nTransactionsUpdated;
}

void CTxMemPool::queryHashes(std::vector<uint256>& vtxid)
{
    vtxid.clear();

    LOCK(cs);
    vtxid.reserve(mapTx.size());
    for (map<uint256, CTransaction>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back((*mi).first);
}

bool CTxMemPool::lookup(uint256 hash, CTransaction& result) const
{
    LOCK(cs);
    std::map<uint256, CTransaction>::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end()) return false;
    result = i->second;
    return true;
}

bool CTxMemPool::WriteFeeEstimates(CAutoFile& fileout) const
{
    try {
        LOCK(cs);
        fileout << 120000; // version required to read: 0.12.00 or later
        fileout << CLIENT_VERSION; // version that wrote the file
        minerPolicyEstimator->Write(fileout);
    }
    catch (const std::exception &) {
        LogPrintf("CTxMemPool::WriteFeeEstimates() : unable to write policy estimator data (non-fatal)");
        return false;
    }
    return true;
}

bool CTxMemPool::ReadFeeEstimates(CAutoFile& filein)
{
    try {
        int nVersionRequired, nVersionThatWrote;
        filein >> nVersionRequired >> nVersionThatWrote;
        if (nVersionRequired > CLIENT_VERSION)
            return error("CTxMemPool::ReadFeeEstimates() : up-version (%d) fee estimate file", nVersionRequired);

        LOCK(cs);
        minerPolicyEstimator->Read(filein, minRelayFee);
    }
    catch (const std::exception &) {
        LogPrintf("CTxMemPool::ReadFeeEstimates() : unable to read policy estimator data (non-fatal)");
        return false;
    }
    return true;
}

bool CTxMemPool::ExistsInMemPool(std::vector<unsigned char> vchToFind, opcodetype type)
{
    for (map<uint256, CTransaction>::iterator mi = mempool.mapTx.begin();
        mi != mempool.mapTx.end(); ++mi)
    {
        CTransaction& tx = (*mi).second;
        if (tx.IsCoinBase())
            continue;
        if (IsAliasOp(type))
        {
            vector<vector<unsigned char> > vvch;
            int op;
            int nOut;

            if (DecodeAliasTx(tx, op, nOut, vvch, -1))
            {
                if (op == type)
                {
                    string vchToFindStr = stringFromVch(vchToFind);
                    string vvchFirstStr = stringFromVch(vvch[0]);
                    if (vvchFirstStr == vchToFindStr)
                    {
                        if (GetTxHashHeight(tx.GetHash()) <= 0)
                            return true;
                    }
                    if (vvch.size() > 1)
                    {
                        string vvchSecondStr = HexStr(vvch[1]);
                        if (vvchSecondStr == vchToFindStr)
                        {
                            if (GetTxHashHeight(tx.GetHash()) <= 0)
                                return true;
                        }
                    }
                }
            }
        }
        else if (IsOfferOp(type))
        {
            vector<vector<unsigned char> > vvch;
            int op;
            int nOut;

           if (DecodeOfferTx(tx, op, nOut, vvch, -1)) 
           {
                if(op == type)
                {
                    string vchToFindStr = stringFromVch(vchToFind);
                    string vvchFirstStr = stringFromVch(vvch[0]);
                    if(vvchFirstStr == vchToFindStr)
                    {
                        if (GetTxHashHeight(tx.GetHash()) <= 0) 
                            return true;
                    }
                    if(vvch.size() > 1)
                    {
                        string vvchSecondStr = HexStr(vvch[1]);
                        if(vvchSecondStr == vchToFindStr)
                        {
                            if (GetTxHashHeight(tx.GetHash()) <= 0)
                                return true;
                        }
                    }
                }
            } 
        } 
        else if(IsCertOp(type))
        {
            vector<vector<unsigned char> > vvch;
            int op;
            int nOut;
        
            if(DecodeCertTx(tx, op, nOut, vvch, -1))   
            {
                if(op == type)
                {
                    string vchToFindStr = stringFromVch(vchToFind);
                    string vvchFirstStr = stringFromVch(vvch[0]);
                    if(vvchFirstStr == vchToFindStr)
                    {
                        if (GetTxHashHeight(tx.GetHash()) <= 0)
                                return true;
                }
                if(vvch.size() > 1)
                {
                    string vvchSecondStr = HexStr(vvch[1]);
                    if(vvchSecondStr == vchToFindStr)
                    {
                        if (GetTxHashHeight(tx.GetHash()) <= 0) 
                            return true;
                    }
                }
            }
        } 
    }
    return false;
}
}


