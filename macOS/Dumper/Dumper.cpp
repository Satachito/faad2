//	https://wiki.multimedia.cx/index.php/Understanding_AAC
//	https://developer.apple.com/library/archive/documentation/QuickTime/QTFF
//	https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html
#include "JPMP4.h"
using namespace JP;

void
ADTS( const vector< UI1 >& adts, UI8 offset = 0 ) {
	//	ADTS Header
	while ( offset < adts.size() ) {
		cout << offset << endl;
		auto _ = adts.data() + offset;
		A( _[ 0 ] == 0xff && ( _[ 1 ] & 0xf0 ) == 0xf0 );
		cout << ( ( _[ 1 ] & 0x08 ) ? "MPEG-2" : "MPEG-4" ) << endl;
		A( ( _[ 1 ] & 0x06 ) == 0 );
		cout << ( ( _[ 1 ] & 0x01 ) ? "NO CRC" : "CRC Exists" ) << endl;
		cout << "Profile: ";
		switch ( ( _[ 2 ] & 0xc0 ) >> 6 ) {
		case 0:	cout << "Main" << endl; break;
		case 1:	cout << "LC" << endl; break;
		case 2:	cout << "SSR" << endl; break;
		case 3:	cout << "LTP" << endl; break;
		default: cout << endl; break;
		}
		switch ( ( _[ 2 ] & 0x3c ) >> 2 ) {
		case  0: cout << "96000Hz" << endl; break;
		case  1: cout << "88200Hz" << endl; break;
		case  2: cout << "64000Hz" << endl; break;
		case  3: cout << "48000Hz" << endl; break;
		case  4: cout << "44100Hz" << endl; break;
		case  5: cout << "32000Hz" << endl; break;
		case  6: cout << "24000Hz" << endl; break;
		case  7: cout << "22050Hz" << endl; break;
		case  8: cout << "16000Hz" << endl; break;
		case  9: cout << "12000Hz" << endl; break;
		case 10: cout << "11025Hz" << endl; break;
		case 11: cout << "8000Hz" << endl; break;
		case 12: cout << "7350Hz" << endl; break;
		case 13: cout << "eh? Hz" << endl; break;
		case 14: cout << "eh? Hz" << endl; break;
		case 15: cout << "IMPLICIT Hz" << endl; break;
		}
		cout << ( ( _[ 2 ] & 0x02 ) ? "Decoding" : "Encoding" ) << endl;
		switch ( ( _[ 2 ] & 0x01 ) << 2 | ( _[ 3 ] & 0xc0 ) >> 6 ) {
		case 0: cout << "in PCE" << endl; break;
		case 1: cout << "FC" << endl; break;
		case 2: cout << "FL:FR" << endl; break;
		case 3: cout << "FC:FL:FR" << endl; break;
		case 4: cout << "FC:FL:FR:BC" << endl; break;
		case 5: cout << "FC:FL:FR:BL:BR" << endl; break;
		case 6: cout << "FC:FL:FR:BL:BR:LFE" << endl; break;
		case 7: cout << "FC:FL:FR:SL:SR:BL:BR:LFE" << endl; break;
		default: cout << "eh?" << endl; break;
		}
		auto frameLength = ( _[ 3 ] & 0x03 ) << 11 | _[ 4 ] << 3 | _[ 5 ] >> 5;
		cout << "Frame length: " << frameLength << endl;
		cout << "Buffer length: " << ( ( _[ 5 ] & 0x1f ) << 6 | _[ 6 ] >> 2 ) << endl;
		cout << "num aac frame?: " << ( _[ 6 ] & 0x03 ) << endl;
		if ( ( _[ 1 ] & 0x01 ) == 0 ) cout << hex << (int)_[ 7 ] << ' ' << (int)_[ 8 ] << endl;

		cout << EncodeHex( _ + 7, 16 );
		
		offset += frameLength;
		cout << endl;
	}
}

UI8
SevenBitsSize( UI1* _, UI8& $ ) {
	$ = _[ 0 ] & 0x7f;
	if ( ( _[ 0 ] & 0x80 ) == 0 ) return 1;
	$ = ( $ << 7 ) | ( _[ 1 ] & 0x7f );
	if ( ( _[ 1 ] & 0x80 ) == 0 ) return 2;
	$ = ( $ << 7 ) | ( _[ 2 ] & 0x7f );
	if ( ( _[ 2 ] & 0x80 ) == 0 ) return 3;
	$ = ( $ << 7 ) | ( _[ 3 ] & 0x7f );
	return 4;
}

struct
AudioInfo {
	UI8	nChannels				= 0;
	UI8	nBits					= 0;
	UI8	sampleRate				= 0;
	UI8	nSamples				= 0;
	UI8	bufferSize				= 0;
	UI8 bitRateMax				= 0;
	UI8 bitRateAvg				= 0;
	UI8 ascSize					= 0;
	UI8 objectType				= 0;
	UI8 channelsConfiguration	= 0;
	UI8 frequencyIndex			= 0;
	UI8 frequency				= 0;
	UI8	frameLength				= 0;

	vector< pair< UI8, UI8 > >	frameInfo;
	
	AudioInfo( const Box& trak ) {
		auto mdia = trak.Only( Q_mdia );
		auto stbl = mdia.Only( Q_minf ).Only( Q_stbl );
		for ( auto& _: stbl.Only( Q_stsd ).children ) {
			if ( _.type == Q_mp4a ) {
				{//	faad2:mp4read.c:esdsin
					auto esds	= _.Only( Q_esds );
					auto data	= esds.data.data() + 4;
//	cerr << EncodeHex( data, 16 ) << endl;
					
					UI8	dummySize;
				//	ES
					A( *data == 3 );					data += 1;
					data += SevenBitsSize( data, dummySize );
//	cerr << "Size: " << size << endl;
//	cerr << "ESID: " << Size2( data ) << endl;
														data += 2;
//	cerr << "flags: " << HexStr( *data ) << endl;
														data += 1;
				//	DC
					A( *data == 4 );					data += 1;
					data += SevenBitsSize( data, dummySize );
//	cerr << "Size: " << size << endl;
					A( *data == 0x40 );					data += 1;	//	MPEG-4 audio
//	cerr << "flags: " << HexStr( *data ) << endl;
														data += 1;
					bufferSize = ( Size2( data ) << 8 ) | data[ 2 ];
														data += 3;
					bitRateMax = Size4( data );			data += 4;
					bitRateAvg = Size4( data );			data += 4;
					
				//	DSI
					A( *data == 5 );					data += 1;
					data += SevenBitsSize( data, ascSize );
					A( ascSize <= 10 );	//	sizeof(mp4config.asc.buf)
					//	https://wiki.multimedia.cx/index.php/Understanding_AAC
					//	mp4.c::AudioSpecificConfigFromBitfile
					cerr << "mp4config.asc.buf: " << EncodeHex( data, ascSize ) << endl;
					
					auto asc = data;
					data += ascSize;
					objectType = asc[ 0 ] >> 3;											//	5 bits
cerr << "_objectType: " << objectType << endl;
					frequencyIndex = ( ( asc[ 0 ] & 0x07 ) << 1 ) | ( asc[ 1 ] >> 7 );	//	4 bits
cerr << "_frequencyIndex: " << frequencyIndex << endl;
					channelsConfiguration = ( asc[ 1 ] & 0x7f ) >> 3;					//	4 bits
cerr << "_extension: " << ( asc[ 1 ] & 0x01 ) << endl;

					UI4 frequencyTable[] = {
						96000, 88200, 64000, 48000, 44100, 32000,
						24000, 22050, 16000, 12000, 11025, 8000
					};
					if ( objectType == 5 || objectType == 29 ) {	//	Extension
						auto _ = ( ( asc[ 1 ] & 0x07 ) << 1 ) | ( asc[ 2 ] >> 7 );		//	4 bits
						if ( _ == frequencyIndex ) cerr << "downSampledSBR" << endl;
						frequencyIndex = _;
cerr << "_frequencyIndex(Extension): " << frequencyIndex << endl;
						if ( frequencyIndex == 15 ) {
							frequency =	( ( asc[ 2 ] & 0x7f ) << 17 ) | ( asc[ 3 ] << 9 ) | ( asc[ 4 ] << 1 ) | ( asc[ 5 ] >> 7 );
							objectType = ( asc[ 5 ] & 0x7f ) >> 2;
							frameLength = ( asc[ 5 ] & 0x02 ) ? 960 : 1024;
						} else {
							frequency =	frequencyTable[ frequencyIndex ];
							objectType = ( asc[ 2 ] & 0x7f ) >> 2;
							frameLength = ( asc[ 2 ] & 0x02 ) ? 960 : 1024;
						}
cerr << "_objectType(Extension): " << objectType << endl;
					} else {
						frequency = frequencyTable[ frequencyIndex ];
						frameLength = ( asc[ 1 ] & 0x04 ) ? 960 : 1024;
					}
				//	SLC
					A( *data == 6 );					data += 1;
					data += SevenBitsSize( data, dummySize );
//	cerr << "Size: " << size << endl;
//	cerr << "Predefined: " << HexStr( *data ) << endl;
														data += 1;
				}

				{	auto data	= _.data.data() + 16;
					nChannels	= Size2( data );		data += 2;
					nBits		= Size2( data );		data += 2;
				}
				
				{	auto mdhd	= mdia.Only( Q_mdhd );
					auto data	= mdhd.data.data() + 12;
					sampleRate	= Size4( data );		data += 4;
					nSamples	= Size4( data );		data += 4;
				}
				{	auto stsz	= stbl.Only( Q_stsz );
					auto data	= stsz.data.data() + 8;
					auto nFrames= Size4( data );		data += 4;
					frameInfo.resize( nFrames );
					for ( UI8 _ = 0; _ < nFrames; _++ ) {
						frameInfo[ _ ].second = Size4( data );	data += 4;
					}
				}
				{	vector< pair< UI8, UI8 > >	sliceInfo;
					{	auto stsc	= stbl.Only( Q_stsc );	//	Slice i.e. Sample to chunk
						auto data	= stsc.data.data() + 4;
						auto nSlices= Size4( data );		data += 4;
						sliceInfo.resize( nSlices );
						for ( UI8 _ = 0; _ < nSlices; _++ ) {
							sliceInfo[ _ ].first = Size4( data );	data += 4;	//	First chunk
							sliceInfo[ _ ].second = Size4( data );	data += 4;	//	Samples per chunk
																	data += 4;	//	Samples description ID
						}
					}
					{	auto stco	= stbl.Only( Q_stco );
						auto data	= stco.data.data() + 4;
						auto nChunks	= Size4( data );	data += 4;	//	101: # of offset data

						for ( UI8 _ = 1; _ < sliceInfo.size(); _++ ) {
							sliceInfo[ _ - 1 ].first = sliceInfo[ _ ].first - sliceInfo[ _ - 1 ].first;
						}
						sliceInfo.back().first = nChunks - sliceInfo.back().first + 1;	//	Index starts with 1, not 0
						
						UI8 iFrame = 0;
						for ( auto& slice: sliceInfo ) {
							for ( UI8 _first = 0; _first < slice.first; _first++ ) {
								auto offset = Size4( data );	data += 4;
								for ( UI8 _second = 0; _second < slice.second; _second++ ) {
									frameInfo[ iFrame ].first = offset;
									offset += frameInfo[ iFrame ].second;
									iFrame++;
								}
							}
						}
						A( iFrame == frameInfo.size() );
					}
				}
			}
		}
	}
	void
	Dump( ostream& s = cerr ) {
		s << "	nChannels				: "	<< nChannels				<< endl;
		s << "	bits					: "	<< nBits					<< endl;
		s << "	sampleRate				: "	<< sampleRate				<< endl;
		s << "	nSamples				: "	<< nSamples					<< endl;
		s << "	# of Frames				: "	<< frameInfo.size()			<< endl;
		s << "	bufferSize				: "	<< bufferSize				<< endl;
		s << "	bitRateMax				: "	<< bitRateMax				<< endl;
		s << "	bitRateAvg				: "	<< bitRateAvg				<< endl;
		s << "	asc.size				: "	<< ascSize					<< endl;
		s << "	objectType				: "	<< objectType				<< endl;
		s << "	channelsConfiguration	: "	<< channelsConfiguration	<< endl;
		s << "	frequency				: "	<< frequency				<< endl;
		s << "	frameLength				: "	<< frameLength				<< endl;
	}
};

struct
MP4 {
	vector< UI1 >	raw;
	AudioInfo*		audioInfo;
	
	~MP4() {
		if ( audioInfo ) delete audioInfo;
	}
	MP4( const vector< UI1 >& raw )
	:	raw( raw )
	,	audioInfo( 0 ) {
		auto mp4 = Box::List( raw );
		for ( auto& _: mp4 ) {
			switch ( _.type ) {
			case Q_moov:
				for ( auto& trak: _.children ) {
					if ( trak.type != Q_trak ) continue;
					for ( auto& _: trak.Only( Q_mdia ).Only( Q_minf ).Only( Q_stbl ).Only( Q_stsd ).children ) {
						switch ( _.type ) {
						case Q_mp4a:
							audioInfo = new AudioInfo( trak );
							break;
						}
					}
				}
				break;
			}
		}
	}
	struct	//	individual channel stream
	ICS {
		UI8 globalGain;
		UI8	windowSequence;
		UI8	windowShape;
		UI8	maxSFB;
		UI8 scaleFactorGrouping;
		UI8 nWindows;
		UI8 nWindowGroups;
		UI8 windowGroupLength[ 8 ];
		UI8 nSWB;

		UI8 sectSFBOffset[ 8 ][ 15 * 8 ];
		UI8 swbOffset[ 52 ];
		UI8 swbOffsetMax;

		UI8 sectCB[ 8 ][ 15 * 8 ];
		UI8 noiseUsed = 0;
		UI8 isUsed = 0;
		UI8 sectStart[ 8 ][ 15 * 8 ];
		UI8 sectEnd[ 8 ][ 15 * 8 ];
		UI8 sfbCB[ 8 ][ 8 * 15 ];
		UI8 numSec[ 8 ]; /* number of sections in a group */
		
 		UI8 scale_factors[ 8 ][ 51 ]; /* [0..255], except for noise and intensity */

		ICS() {
		}

		//	#define ONLY_LONG_SEQUENCE   0x0
		//	#define LONG_START_SEQUENCE  0x1
		//	#define EIGHT_SHORT_SEQUENCE 0x2
		//	#define LONG_STOP_SEQUENCE   0x3

		ICS( BitReader& br, const AudioInfo* audioInfo ) {
			A( !br.Read() );
			windowSequence = br.Read( 2 );
			windowShape = br.Read( 1 );	//	0: Sine / 1:Kaiser Bessel
			nWindowGroups = 1;
			windowGroupLength[ 0 ] = 1;
			switch ( windowSequence ) {
			case 2:	//	EIGHT_SHORT_SEQUENCE
				maxSFB = br.Read( 4 );
				scaleFactorGrouping = br.Read( 7 );
				break;
			default:
				maxSFB = br.Read( 6 );
				nWindows = 1;
				if ( br.Read( 1 ) ) {	//	predictor_data_present
					//	TODO:
				}
//				static UI8 num_swb_512_window[] = { 0, 0, 0, 36, 36, 37, 31, 31, 0, 0, 0, 0 };	//	LD
//				static UI8 num_swb_480_window[] = { 0, 0, 0, 35, 35, 37, 30, 30, 0, 0, 0, 0 };	//	LD
				static UI8 num_swb_960_window[] = { 40, 40, 45, 49, 49, 49, 46, 46, 42, 42, 42, 40 };
				static UI8 num_swb_1024_window[] = { 41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40 };
				const UI8* window = 0;
				switch ( audioInfo->frameLength ) {
				case 1024	:	window = num_swb_1024_window;	break;
				case 960	:	window = num_swb_960_window;	break;
//				case 512	:	window = num_swb_512_window;	break;	//	LD
//				case 480	:	window = num_swb_480_window;	break;	//	LD
				}
				nSWB = window[ audioInfo->frequencyIndex ];
				static UI8 swb_offset_1024_96[] = {
					0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
					64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
					276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
				};
				static UI8 swb_offset_1024_64[] = {
					0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
					64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
					304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
					864, 904, 944, 984, 1024
				};
				static UI8 swb_offset_1024_48[] = {
					0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
					80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
					320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
					768, 800, 832, 864, 896, 928, 1024
				};
				static UI8 swb_offset_1024_32[] =
				{
					0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
					80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
					320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
					768, 800, 832, 864, 896, 928, 960, 992, 1024
				};
				static UI8 swb_offset_1024_24[] =
				{
					0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
					76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
					240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
					768, 832, 896, 960, 1024
				};
				static UI8 swb_offset_1024_16[] =
				{
					0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
					136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
					368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
				};
				static UI8 swb_offset_1024_8[] =
				{
					0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
					188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
					476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
				};
				static UI8*swb_offset_1024_window[] = {
					swb_offset_1024_96,      /* 96000 */
					swb_offset_1024_96,      /* 88200 */
					swb_offset_1024_64,      /* 64000 */
					swb_offset_1024_48,      /* 48000 */
					swb_offset_1024_48,      /* 44100 */
					swb_offset_1024_32,      /* 32000 */
					swb_offset_1024_24,      /* 24000 */
					swb_offset_1024_24,      /* 22050 */
					swb_offset_1024_16,      /* 16000 */
					swb_offset_1024_16,      /* 12000 */
					swb_offset_1024_16,      /* 11025 */
					swb_offset_1024_8        /* 8000  */
				};
				for ( UI8 _ = 0; _ < nSWB; _++ ) {
					sectSFBOffset[ 0 ][ _ ] = swb_offset_1024_window[ audioInfo->frequencyIndex ][ _ ];
					swbOffset[ _ ] = swb_offset_1024_window[ audioInfo->frequencyIndex ][ _ ];
				}
				sectSFBOffset[ 0 ][ nSWB ] = audioInfo->frameLength;
				swbOffset[ nSWB ] = audioInfo->frameLength;
				swbOffsetMax = audioInfo->frameLength;
			}
		}
		void
		Supp( BitReader& br, const AudioInfo* audioInfo ) {
			globalGain = br.Read( 8 );
//	SECTION DATA
			UI8	sect_bits;
			UI8	sect_lim;	 // 51 or 120, anyways less than 127.
			switch ( windowSequence ) {
			case 2:	//	EIGHT_SHORT_SEQUENCE
				sect_bits = 3;
				sect_lim = 8 * 15;
				break;
			default:
				sect_bits = 5;
				sect_lim = maxSFB;
			}
			UI8 sect_esc_val = ( 1 << sect_bits ) - 1;

			for ( UI8 _ = 0; _ < nWindowGroups; _++ ) {
				uint8_t k = 0;
				uint8_t i = 0;
				while ( k < maxSFB ) {
					sectCB[ _ ][ i ] = br.Read( 4 );	//	FAAD2: sect_cb_bits

					switch ( sectCB[ _ ][ i ] ) {
					case 13:	//	NOISE_HCB
						noiseUsed = 1;
						break;
					case 14:	//	INTENSITY_HCB2
					case 15:	//	INTENSITY_HCB
						isUsed = 1;
						break;
					}

					UI8 sect_len_incr = br.Read( sect_bits );
					UI8 sect_len = 0;
					while ( sect_len_incr == sect_esc_val ) {
						sect_len += sect_len_incr;
						sect_len_incr = br.Read( sect_bits );
					}
					sect_len += sect_len_incr;

					sectStart[ _ ][ i ] = k;
					sectEnd[ _ ][ i ] = k + sect_len;

					for ( UI8 sfb = k; sfb < k + sect_len; sfb++ ) sfbCB[ _ ][ sfb ] = sectCB[ _ ][ i ];

					k += sect_len; // k <= sect_lim
					i++;
				}
				numSec[ _ ] = i;
			}
		    
//	SCALE FACTOR DATA




			if ( br.Read() ) {	//	pulse_data_present
				//	TODO
			}
			if ( br.Read() ) {	//	tns_data_present
				//	TODO
			}
			if ( br.Read() ) {	//	gain_control_data_present
				//	TODO
			}
		//	TODO: SPECTRAL DATA
		}
	};
	void
	Analyze( const vector< UI1 >& _ ) {
/*
0 SCE single channel element (codes a single audio channel)
1 CPE channel pair element (codes stereo signal)
2 CCE something to do with channel coupling, not implemented in libfaad2
3 LFE low-frequency effects? referenced as "special effects" in RTP doc
4 DSE data stream element (user data)
5 PCE program configuration element (describe bitstream)
6 FIL fill element (pad space/extension data)
7 END marks the end of the frame

This is an example layout for a 5.1 audio stream:
SCE CPE CPE LFE END

*/
		BitReader	br( _.data() );
		while ( br._ < _.size() * 8 ) {
			auto type = br.Read( 3 );
			br.Read( 4 );	//	element ID
			switch ( type ) {
			case 0:	//	ID_SCE
cerr << "ID_SCE" << endl;
				{	ICS	ics( br, audioInfo );
					ics.Supp( br, audioInfo );
				}
				break;
			case 1:	//	channel_pair_element
cerr << "ID_CPE" << endl;
				{	ICS ics1;
					ICS ics2;
					auto maxIDX = ics1.nWindowGroups * ics1.maxSFB;
					UI8 msMask[ maxIDX ];
					auto commonWindow = br.Read( 1 );
					if ( commonWindow ) {
						ics1 = ICS( br, audioInfo );
						switch ( br.Read( 2 ) ) {	//	decode_mid_side_stereo
						case 1:
							for ( auto _ = 0; _ < maxIDX; _++ ) msMask[ _ ] = br.Read( 1 );
							break;
						case 2:
							for ( auto _ = 0; _ < maxIDX; _++ ) msMask[ _ ] = 1;
							break;
						case 3:
							throw "eh?";
						}
						ics2 = ics1;
					} else {
					//	ここほんとにうまくいくの？か確認
					//	ics1->ms_mask_present;
						ics2 = ICS( br, audioInfo );
					}
					ics1.Supp( br, audioInfo );
					ics2.Supp( br, audioInfo );
				}
				break;
			case 2:	//	ID_CCE
cerr << "ID_CCE" << endl;
				break;
			case 3:	//	ID_LFE
cerr << "ID_LFE" << endl;
				break;
			case 4:	//	ID_DSE
cerr << "ID_DSE" << endl;
				break;
			case 5:	//	ID_PCE
cerr << "ID_PCE" << endl;
				break;
			case 6:	//	ID_FIL
cerr << "ID_FIL" << endl;
				{	auto count = br.Read( 4 );
					if ( count == 15 ) count += br.Read( 8 ) - 1;
					while ( count ) {
						switch ( br.Read( 4 ) ) {
					//	#define EXT_FIL            0
					//	#define EXT_FILL_DATA      1
					//	#define EXT_DATA_ELEMENT   2
					//	#define EXT_DYNAMIC_RANGE 11
						case 11:	//	EXT_DYNAMIC_RANGE:
							break;
						default:	//	EXT_FIL
							br.Skip( 8 * count - 4 );	//	Align
							count = 0;
							break;
						}
					}
				}
				break;
			case 7:	//	ID_END
cerr << "ID_END" << endl;
				break;
			default:
				throw "eh?";
			}
		}
	}
	vector< UI1 >
	ExtractAAC() {
		vector< UI1 >	$;
		for ( auto& frame: audioInfo->frameInfo ) {
cerr << frame.second << endl;
//Analyze( vector< UI1 >( &raw[ frame.first ], &raw[ frame.first + frame.second ] ) );
			$.insert( $.end(), &raw[ frame.first ], &raw[ frame.first + frame.second ] );
		}
		return $;
	}
};

void
Main( const vector< UI1 >& _ ) {
	MP4 $( _ );
	$.audioInfo->Dump();
	$.ExtractAAC();
}

int main(int argc, const char * argv[]) {
	chdir( "/Volumes/upStream/AAC" );
//	ADTS( GetFileContent( "$.aac" ) );
	Main( GetFileContent( "ss.m4a" ) );
//	Main( GetFileContent( "AAC 5.1.mp4" ) );
	return 0;
}
