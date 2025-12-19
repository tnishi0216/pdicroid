//---------------------------------------------------------------------------

#ifndef DicDecH
#define DicDecH
//---------------------------------------------------------------------------

class TDicDec {
protected:
	class TUnzip *unzip;
	void *huz;
public:
	TDicDec();
	~TDicDec();
	static bool IsDicDecIndex(const struct HEADER *header);
	static bool IsDicDecData(const struct HEADER *header);
	unsigned GetDecodeSize(const char *data);
	unsigned GetDecodeSizeForDB(const char *data);
	unsigned Decode(const char *data, uint8_t *out);
	void DecodeForDB(const char *data, uint8_t *out);
};

#endif

