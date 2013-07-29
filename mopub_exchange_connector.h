 /* mopub_exchange_connector.h     -*- C++ -*-
   Syed Murtaza Hussain Kazmi, 21 June 2013
   Copyright (c) 2013 Virtual Force PVT.  All rights reserved.

*/

#pragma once

#include "rtbkit/plugins/exchange/http_exchange_connector.h"

namespace RTBKIT {


/*****************************************************************************/
/* MOPUB EXCHANGE CONNECTOR                                                */
/*****************************************************************************/

/** Exchange connector for MoPub.  This speaks their flavour of the
    OpenRTB 2.1 protocol.

    Configuration options are the same as the HttpExchangeConnector on which
    it is based.
*/

struct MoPubExchangeConnector: public HttpExchangeConnector {
    MoPubExchangeConnector(ServiceBase & owner, const std::string & name);
    MoPubExchangeConnector(const std::string & name,
                             std::shared_ptr<ServiceProxies> proxies);

    static std::string exchangeNameString() {
        return "mopub";
    }

    virtual std::string exchangeName() const {
        return exchangeNameString();
    }

    virtual std::shared_ptr<BidRequest>
    parseBidRequest(HttpAuctionHandler & connection,
                    const HttpHeader & header,
                    const std::string & payload);

    virtual double
    getTimeAvailableMs(HttpAuctionHandler & connection,
                       const HttpHeader & header,
                       const std::string & payload);

  /*virtual double getRoundTripTimeMs(HttpAuctionHandler & handler,
                          const HttpHeader & header) ;*/

    virtual HttpResponse
    getResponse(const HttpAuctionHandler & connection,
                const HttpHeader & requestHeader,
                const Auction & auction) const;

    virtual HttpResponse
    getDroppedAuctionResponse(const HttpAuctionHandler & connection,
                              const std::string & reason) const;

    virtual HttpResponse
    getErrorResponse(const HttpAuctionHandler & connection,
                     const Auction & auction,
                     const std::string & errorMessage) const;

    /** This is the information that the MoPub exchange needs to keep
        for each campaign (agent).
    */
    struct CampaignInfo {
        Id seat;       ///< ID of the MoPub exchange seat
    };

    virtual ExchangeCompatibility
    getCampaignCompatibility(const AgentConfig & config,
                             bool includeReasons) const;
    
    /** This is the information that MoPub needs in order to properly
        filter and serve a creative.
    */
    struct CreativeInfo {
        std::string adm;                  ///< Ad markup
        std::vector<std::string> adomain; ///< Advertiser domains
        Id crid;                          ///< Creative ID
        std::string iurl;                ///< The iURL should be a representation of the creative which is being served. 
        Id adid;                          ///< Ad ID
        OpenRTB::List<OpenRTB::CreativeAttribute> attr; ///< Creative attributes
		
        std::string ext_creativeapi;      ///< Creative API
    };

    virtual ExchangeCompatibility
    getCreativeCompatibility(const Creative & creative,
                             bool includeReasons) const;

    // MoPub win price decoding function.
    static float decodeWinPrice(const std::string & sharedSecret,
                                const std::string & winPriceStr);
};



} // namespace RTBKIT

