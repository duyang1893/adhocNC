
#include <ctime>
#include <set>

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>
#include <kodo/set_systematic_on.hpp>
#include <kodo/set_systematic_off.hpp>


int main()
{
     srand(static_cast<uint32_t>(time(0)));

    uint32_t symbols = 5;
    uint32_t symbol_size =20;

    typedef kodo::full_rlnc_encoder<fifi::binary8, kodo::disable_trace>  rlnc_encoder;
    typedef kodo::full_rlnc_decoder<fifi::binary8, kodo::enable_trace>  rlnc_decoder;

    rlnc_encoder::factory encoder_factory(symbols, symbol_size);
    auto encoder = encoder_factory.build();
    kodo::set_systematic_on(encoder);
    rlnc_decoder::factory decoder_factory(symbols, symbol_size);
    auto decoder = decoder_factory.build();


	std::vector<uint8_t> codedPacket(symbol_size+1+symbols);
	std::vector<uint8_t> data(symbol_size*symbols);
        std::generate_n(begin(data), data.size(), rand);
        encoder->set_symbols(sak::storage(data));

	uint8_t *symbol_data = codedPacket.data();
	uint8_t *genId = codedPacket.data()+symbol_size;
	genId[0]=1;
        uint8_t *symbol_id   =  codedPacket.data()+symbol_size+1;
	uint8_t *coefficients = 0;

std::cout<<"encoder->symbols()=="<<encoder->symbols()<<std::endl;
std::cout<<"encoder->symbols_uncoded()=="<<encoder->symbols_uncoded()<<std::endl;
//std::cout<<"encoder->symbols_seen()=="<<encoder->symbols_seen()<<std::endl;
//std::cout<<"encoder->symbols_missing()=="<<encoder->symbols_missing()<<std::endl;
uint32_t j=0;
   while ( j<5 )
    {

	encoder->write_id(symbol_id,&coefficients);
	for (uint32_t i=0; i<symbols; i++)
	{
		if (i>j)
		{
			symbol_id[i]=0;
		}
	}
	encoder->encode_symbol(symbol_data,symbol_id);
uint32_t i=0;
for (i=0;i<codedPacket.size(); i++)
{
std::cout<<"codedPacket "<<i<<" "<<uint32_t(codedPacket[i])<<std::endl;
}
std::cout<<std::endl;

      decoder->decode(codedPacket.data());

        if (kodo::has_trace<rlnc_decoder>::value)
        {
           auto filter = [](const std::string& zone)
            {
                std::set<std::string> filters =
                    {"decoder_state", "input_symbol_coefficients"};

                return filters.count(zone);
            };

            std::cout << "Trace decoder:" << std::endl;


            // Try to run without a filter to see the full amount of
            // output produced by the trace function. You can then
            // modify the filter to only view the information you are
            // interested in.

            kodo::trace(decoder, std::cout, filter);
            //kodo::trace(decoder, std::cout);
        }
	j++;
std::cout<<"encoder->symbols_uncoded()=="<<encoder->symbols_uncoded()<<std::endl;
//std::cout<<"encoder->symbols_seen()=="<<encoder->symbols_seen()<<std::endl;
//std::cout<<"encoder->symbols_missing()=="<<encoder->symbols_missing()<<std::endl;

   }

    std::vector<uint8_t> data_out(decoder->block_size());
    decoder->copy_symbols(sak::storage(data_out));

    // Check we properly decoded the data
    if (std::equal(data_out.begin(), data_out.end(), data.begin()))
    {
        std::cout << "Data decoded correctly" << std::endl;
    }
    else
    {
        std::cout << "Unexpected failure to decode "
                  << "please file a bug report :)" << std::endl;
    }

}

/*std::cout<<"encoder functions are"<<std::endl;
std::cout<<"encoder->symbol_size()="<<encoder->symbol_size()<<std::endl;
std::cout<<"encoder->header_size()="<<encoder->header_size()<<std::endl; 
std::cout<<"encoder->payload_size()="<<encoder->payload_size()<<std::endl;
std::cout<<"encoder->id_size()="<<encoder->id_size()<<std::endl;
std::cout<<"encoder->coefficient_vector_size()="<<encoder->coefficient_vector_size()<<std::endl;
std::cout<<"encoder->symbols()="<<encoder->symbols()<<std::endl;
std::cout<<"encoder->block_size()="<<encoder->block_size()<<std::endl;*/
