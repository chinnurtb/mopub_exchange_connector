 /* mopub_exchange_connector.cc                                -*- C++ -*-
   Syed Murtaza Hussain Kazmi, 21 June 2013
   Copyright (c) 2013 Virtual Force PVT.  All rights reserved.
*/

#include "mopub_exchange_connector.h"
#include "rtbkit/plugins/bid_request/openrtb_bid_request.h"
#include "rtbkit/plugins/exchange/http_auction_handler.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "openrtb/openrtb_parsing.h"
#include "soa/types/json_printing.h"
#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include "jml/utils/file_functions.h"

#include "crypto++/blowfish.h"
#include "crypto++/modes.h"
#include "crypto++/filters.h"

using namespace std;
using namespace Datacratic;

namespace Datacratic {

template<typename T, int I, typename S>
Json::Value jsonEncode(const ML::compact_vector<T, I, S> & vec)
{
    Json::Value result(Json::arrayValue);
    for (unsigned i = 0;  i < vec.size();  ++i)
        result[i] = jsonEncode(vec[i]);
    return result;
}

template<typename T, int I, typename S>
ML::compact_vector<T, I, S>
jsonDecode(const Json::Value & val, ML::compact_vector<T, I, S> *)
{
    ExcAssert(val.isArray());
    ML::compact_vector<T, I, S> res;
    res.reserve(val.size());
    for (unsigned i = 0;  i < val.size();  ++i)
        res.push_back(jsonDecode(val[i], (T*)0));
    return res;
}


} // namespace Datacratic

namespace OpenRTB {

template<typename T>
Json::Value jsonEncode(const OpenRTB::List<T> & vec)
{
    using Datacratic::jsonEncode;
    Json::Value result(Json::arrayValue);
    for (unsigned i = 0;  i < vec.size();  ++i)
        result[i] = jsonEncode(vec[i]);
    return result;
}

template<typename T>
OpenRTB::List<T>
jsonDecode(const Json::Value & val, OpenRTB::List<T> *)
{
    using Datacratic::jsonDecode;
    ExcAssert(val.isArray());
    OpenRTB::List<T> res;
    res.reserve(val.size());
    for (unsigned i = 0;  i < val.size();  ++i)
        res.push_back(jsonDecode(val[i], (T*)0));
    return res;
}

} // namespace OpenRTB

namespace RTBKIT {

BOOST_STATIC_ASSERT(hasFromJson<Datacratic::Id>::value == true);
BOOST_STATIC_ASSERT(hasFromJson<int>::value == false);


/*****************************************************************************/
/* MoPub EXCHANGE CONNECTOR                                                */
/*****************************************************************************/

MoPubExchangeConnector::
MoPubExchangeConnector(ServiceBase & owner, const std::string & name)
    : HttpExchangeConnector(name, owner)
{
}

MoPubExchangeConnector::
MoPubExchangeConnector(const std::string & name,
                         std::shared_ptr<ServiceProxies> proxies)
    : HttpExchangeConnector(name, proxies)
{
    this->auctionResource = "/auctions";
    this->auctionVerb = "POST";
}


/**
 *  Parse Bid Request
 * 
 */

std::shared_ptr<BidRequest>
MoPubExchangeConnector::
parseBidRequest(HttpAuctionHandler & connection,
                const HttpHeader & header,
                const std::string & payload)
{
    std::shared_ptr<BidRequest> res;

    // Check for JSON content-type
    if (header.contentType != "application/json") {
        connection.sendErrorResponse("non-JSON request");
        return res;
    }


    // Parse the bid request
    ML::Parse_Context context("Bid Request", payload.c_str(), payload.size());
    res.reset(OpenRtbBidRequestParser::parseBidRequest(context, exchangeName(), exchangeName()));
        
    cerr << res->toJson() << endl;

    return res;
}




/**     Return the available time for the bid request in milliseconds.
        This method should not parse the bid request, as when 
        shedding load we want to do as little work as possible.
       
*/

double
MoPubExchangeConnector::
getTimeAvailableMs(HttpAuctionHandler & connection,
                   const HttpHeader & header,
                   const std::string & payload)
{    
 
  //each bidder must respond within 200 ms total roundtrip
  // Here value has been fixed as there is no parameter "tmax" mentioned in MoPub documentation
    return 200.0;
}

/**
  An estimation of the round trip time to account for the network latency

*/

	/*double 
	MoPubExchangeConnector::getRoundTripTimeMs(HttpAuctionHandler & handler,
                          const HttpHeader & header) {
    return 200.0;
}*/

/**
 *  Bid Response
 *
 * 
*/

HttpResponse
MoPubExchangeConnector::
getResponse(const HttpAuctionHandler & connection,
            const HttpHeader & requestHeader,
            const Auction & auction) const
{
    const Auction::Data * current = auction.getCurrentData();

    if (current->hasError())
        return getErrorResponse(connection, auction, current->error + ": " + current->details);
    
    OpenRTB::BidResponse response;
    response.id = auction.id;

    std::map<Id, int> seatToBid;

    string en = exchangeName();
    
    // begin the JSON string that will be returned
    cerr<< current->responses.size() <<"size" << endl; 

    // Create a spot for each of the bid responses
    for (unsigned spotNum = 0; spotNum < current->responses.size(); ++spotNum) 
    {
     
	 if (!current->hasValidResponse(spotNum))
            continue;
        // Get the winning bid
        auto & resp = current->winningResponse(spotNum);

        // Find how the agent is configured.  We need to copy some of the
        // fields into the bid.
        const AgentConfig * config
            = std::static_pointer_cast<const AgentConfig>
            (resp.agentConfig).get();

        // Get the exchange specific data for this campaign
        auto cpinfo = config->getProviderData<CampaignInfo>(en);

        // Put in the fixed parts from the creative
        int creativeIndex = resp.agentCreativeIndex;

        auto & creative = config->creatives.at(creativeIndex);

        // Get the exchange specific data for this creative
        auto crinfo = creative.getProviderData<CreativeInfo>(en);
        
        //----------------------------------------------------------------------
        //Open the following lines in the case seatToBid is required.
        
        
        
        // Find the index in the seats array
        int seatIndex = -1;
        {
            auto it = seatToBid.find(cpinfo->seat);
            if (it == seatToBid.end()) {
                seatIndex = seatToBid.size();
                seatToBid[cpinfo->seat] = seatIndex;
                response.seatbid.emplace_back();
                response.seatbid.back().seat = cpinfo->seat;
            }
            else seatIndex = it->second;
        }
        
//        // Get the seatBid object
        OpenRTB::SeatBid & seatBid = response.seatbid.at(seatIndex);
        
        // Add a new bid to the array
        seatBid.bid.emplace_back();
        auto & b = seatBid.bid.back();
        
        // Put in the variable parts
        b.id        =   Id(auction.id, auction.request->imp[0].id); //Bidder's bid ID to identify bid. 
        b.impid     =   auction.request->imp[spotNum].id; // ID of the impression we're bidding on. 
        b.price.val =   USD_CPM(resp.price.maxPrice); //Price
        b.adid      =   Id(crinfo->adid); //Id of ad to be served if won. 
        b.nurl      =  "http://54.214.251.166:18143"; //Win notice/ad markup URL. For each bid, the “nurl” attribute contains the win notice URL. If the bidder wins the impression, the exchange calls this notice URL a) to inform the bidder of the win and b) to convey certain information using substitution macros
        b.adm       =   crinfo->adm; //Ad markup.
        b.adomain   =   crinfo->adomain; //Advertiser domains.
        b.iurl      =   crinfo->iurl; //parameter per the OpenRTB spec.The "iurl" parameter should be a representation of the creative which you're serving. 
		
        b.cid       =   Id(resp.agent); //Campaign ID. 
        b.crid      =   crinfo->crid; //Creative ID. 
        b.attr      =   crinfo->attr; 
      
        //----------------------------------------------------------------------
       
    }

    if (seatToBid.empty())
        return HttpResponse(204, "none", "");

    static Datacratic::DefaultDescription<OpenRTB::BidResponse> desc;
    std::ostringstream stream;
    StreamJsonPrintingContext context(stream);
    desc.printJsonTyped(&response, context);

    cerr << Json::parse(stream.str());
   
    return HttpResponse(200, "application/json", stream.str());
   // return HttpResponse(200, "application/json", result);
}

/**
 *  getDroppedAuctionResponse
 */

HttpResponse
MoPubExchangeConnector::
getDroppedAuctionResponse(const HttpAuctionHandler & connection,
                          const std::string & reason) const
{
    return HttpResponse(204, "application/json", "{}");
}

HttpResponse
MoPubExchangeConnector::
getErrorResponse(const HttpAuctionHandler & connection,
                 const Auction & auction,
                 const std::string & errorMessage) const
{
    Json::Value response;
    response["error"] = errorMessage;
    return HttpResponse(400, response);
}

ExchangeConnector::ExchangeCompatibility
MoPubExchangeConnector::
getCampaignCompatibility(const AgentConfig & config,
                         bool includeReasons) const
{
    ExchangeCompatibility result;
    result.setCompatible();

    auto cpinfo = std::make_shared<CampaignInfo>();

    const Json::Value & pconf = config.providerConfig["mopub"];

    try {
        cpinfo->seat = Id(pconf["seat"].asString());
        if (!cpinfo->seat)
            result.setIncompatible("providerConfig.mopub.seat is null",
                                   includeReasons);
    } catch (const std::exception & exc) {
        result.setIncompatible
            (string("providerConfig.mopub.seat parsing error: ")
             + exc.what(), includeReasons);
        return result;
    }
    
    result.info = cpinfo;
    
    return result;
}

namespace {

using Datacratic::jsonDecode;

/** Given a configuration field, convert it to the appropriate JSON */
template<typename T>
void getAttr(ExchangeConnector::ExchangeCompatibility & result,
             const Json::Value & config,
             const char * fieldName,
             T & field,
             bool includeReasons)
{
    try {
        if (!config.isMember(fieldName)) {
            result.setIncompatible
                ("creative[].providerConfig.mopub." + string(fieldName)
                 + " must be specified", includeReasons);
            return;
        }
        
        const Json::Value & val = config[fieldName];
        
        jsonDecode(val, field);
    }
    catch (const std::exception & exc) {
        result.setIncompatible("creative[].providerConfig.mopub."
                               + string(fieldName) + ": error parsing field: "
                               + exc.what(), includeReasons);
        return;
    }
}
    
} // file scope

ExchangeConnector::ExchangeCompatibility
MoPubExchangeConnector::
getCreativeCompatibility(const Creative & creative,
                         bool includeReasons) const
{
    ExchangeCompatibility result;
    result.setCompatible();

    auto crinfo = std::make_shared<CreativeInfo>();

    const Json::Value & pconf = creative.providerConfig["mopub"];

    // 1.  Must have mopub.attr containing creative attributes.  These
    //     turn into MoPubCreativeAttribute filters.
    getAttr(result, pconf, "attr", crinfo->attr, includeReasons);

    // TODO: create filter from these...

    // 2.  Must have mopub.adm that includes MoPub's macro
    getAttr(result, pconf, "adm", crinfo->adm, includeReasons);
    if (crinfo->adm.find("${AUCTION_PRICE:BF}") == string::npos)
        result.setIncompatible
            ("creative[].providerConfig.mopub.adm ad markup must contain "
             "encrypted win price macro ${AUCTION_PRICE:BF}",
             includeReasons);
    
    // 3.  Must have creative ID in mopub.crid
    getAttr(result, pconf, "crid", crinfo->crid, includeReasons);
    if (!crinfo->crid)
        result.setIncompatible
            ("creative[].providerConfig.mopub.crid is null",
             includeReasons);
            
    // 4.  Must have advertiser names array in mopub.adomain
    getAttr(result, pconf, "adomain", crinfo->adomain,  includeReasons);
    if (crinfo->adomain.empty())
        result.setIncompatible
            ("creative[].providerConfig.mopub.adomain is empty",
             includeReasons);
	//5. Get iurl
	getAttr(result, pconf, "iurl", crinfo->iurl,  includeReasons);
	if (crinfo->iurl.empty())
        result.setIncompatible
            ("creative[].providerConfig.mopub.iurl is empty",
             includeReasons);

	//6. Get AdID
	getAttr(result, pconf, "adid", crinfo->adid,  includeReasons);
	
    // Cache the information
    result.info = crinfo;

    return result;
}

float
MoPubExchangeConnector::
decodeWinPrice(const std::string & sharedSecret,
               const std::string & winPriceStr)
{
    ExcAssertEqual(winPriceStr.length(), 16);
        
    auto tox = [] (char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            else if (c >= 'A' && c <= 'F')
                return 10 + c - 'A';
            else if (c >= 'a' && c <= 'f')
                return 10 + c - 'a';
            throw ML::Exception("invalid hex digit");
        };

    unsigned char input[8];
    for (unsigned i = 0;  i < 8;  ++i)
        input[i]
            = tox(winPriceStr[i * 2]) * 16
            + tox(winPriceStr[i * 2 + 1]);
        
    CryptoPP::ECB_Mode<CryptoPP::Blowfish>::Decryption d;
    d.SetKey((byte *)sharedSecret.c_str(), sharedSecret.size());
    CryptoPP::StreamTransformationFilter
        filt(d, nullptr,
             CryptoPP::StreamTransformationFilter::NO_PADDING);
    filt.Put(input, 8);
    filt.MessageEnd();
    char recovered[9];
    size_t nrecovered = filt.Get((byte *)recovered, 8);

    ExcAssertEqual(nrecovered, 8);
    recovered[nrecovered] = 0;

    float res = boost::lexical_cast<float>(recovered);

    return res;
}

} // namespace RTBKIT

