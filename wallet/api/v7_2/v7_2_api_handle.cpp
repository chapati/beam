// Copyright 2022 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "v7_2_api.h"
#include "version.h"
#include "bvm/bvm2.h"

namespace beam::wallet
{
#ifdef BEAM_ASSET_SWAP_SUPPORT
    void V72Api::onHandleAssetsSwapOffersList(const JsonRpcId& id, AssetsSwapOffersList&& req)
    {
        AssetsSwapOffersList::Response resp;
        resp.orders = getDexBoard()->getDexOrders();

        doResponse(id, resp);
    }

    void V72Api::onHandleAssetsSwapCreate(const JsonRpcId& id, AssetsSwapCreate&& req)
    {
        auto walletDB = getWalletDB();

        std::string sendAssetUnitName = "BEAM";
        if (req.sendAsset)
        {
            auto sendAssetInfo = walletDB->findAsset(req.sendAsset);
            if (!sendAssetInfo)
            {
                throw jsonrpc_exception(ApiError::InvalidParamsJsonRpc, "Unknown asset id - send_asset_id");
            }
            const auto &sendAssetMeta = WalletAssetMeta(*sendAssetInfo);
            sendAssetUnitName = sendAssetMeta.GetUnitName();
        }

        std::string receiveAssetUnitName = "BEAM";
        if (req.receiveAsset)
        {
            auto receiveAssetInfo = walletDB->findAsset(req.receiveAsset);
            if (!receiveAssetInfo)
            {
                throw jsonrpc_exception(ApiError::InvalidParamsJsonRpc, "Unknown asset id - receive_asset_id");
            }
            const auto &receiveAssetMeta = WalletAssetMeta(*receiveAssetInfo);
            receiveAssetUnitName = receiveAssetMeta.GetUnitName();
        }

        CoinsSelectionInfo csi;
        csi.m_requestedSum = req.sendAmount;
        csi.m_assetID = req.sendAsset;
        csi.m_explicitFee = 0;
        csi.Calculate(walletDB->getCurrentHeight(), walletDB, false);

        if (!csi.m_isEnought)
        {
            throw jsonrpc_exception(ApiError::AssetSwapNotEnoughtFunds);
        }

        if (req.expireMinutes < 30 || req.expireMinutes > 720)
        {
            throw jsonrpc_exception(ApiError::InvalidParamsJsonRpc, "minutes_before_expire must be > 30 and < 720");
        }

        WalletAddress receiverAddress;
        walletDB->createAddress(receiverAddress);
        receiverAddress.m_label = req.comment;
        receiverAddress.m_duration = beam::wallet::WalletAddress::AddressExpirationAuto;
        walletDB->saveAddress(receiverAddress);

        beam::wallet::DexOrder orderObj(
            beam::wallet::DexOrderID::generate(),
            receiverAddress.m_walletID,
            receiverAddress.m_OwnID,
            req.sendAsset,
            req.sendAmount,
            sendAssetUnitName,
            req.receiveAsset,
            req.receiveAmount,
            receiveAssetUnitName,
            req.expireMinutes * 60
        );

        getDexBoard()->publishOrder(orderObj);

        AssetsSwapCreate::Response resp;
        resp.order = orderObj;

        doResponse(id, resp);
    }
#endif  // BEAM_ASSET_SWAP_SUPPORT
}
