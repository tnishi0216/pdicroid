//---------------------------------------------------------------------------

#ifndef DllDicTH
#define DllDicTH
//---------------------------------------------------------------------------

#include "DllDic.h"
#include "MemObj.h"

// DllDictionaryT //
template <class T>
class DllDictionaryT : public DllDictionary {
protected:
	bool EPWingSupported;
public:
	DllDictionaryT();
	virtual ~DllDictionaryT();
	int ReadObject(JapaT<T> &japa);
	int SetObject(JapaT<T> &japa);
	virtual TObjDataT<T> *CreateObjData() = 0;
	virtual int sizeOfChar() const
		{ return sizeof(T); }
};

template <class T>
DllDictionaryT<T>::DllDictionaryT()
{
	EPWingSupported = false;
}
template <class T>
DllDictionaryT<T>::~DllDictionaryT()
{
}
template <class T>
int DllDictionaryT<T>::ReadObject(JapaT<T> &japa)
{
	for(int i=0;i<pdcdata.objnum;i++){
		HPDOBJ hobj = pdcdata.object[i];
		int objtype = fnPDGetObjectType(hobj);
		switch (objtype){
			case PDCOBJ_BINARY:
			case PDCOBJ_OLE:
			case PDCOBJ_FILE:
//			case PDCOBJ_VOICE:
//			case PDCOBJ_IMAGE:
				break;
			case PDCOBJ_EPWING:
				if (EPWingSupported){
					break;
				} else {
					continue;
				}
			default:
				continue;
		}
		TObjDataT<T> *obj = CreateObjData();
		if (!obj)
			return -1;
		obj->objtype = objtype;
		obj->id = fnPDGetObjectId(hobj);
		int len;
		if (objtype!=PDCOBJ_EPWING){
			// get title
			len = fnPDGetObjTitle(hobj, NULL);
			T *title = new T[len+1];
			if (!title){
				goto jerror;
			}
			fnPDGetObjTitle(hobj, title);
			obj->title = title;
		} else {
			obj->title = NULL;
		}
		if (objtype==PDCOBJ_FILE){
			// get filename
			len = fnPDGetFileLinkName(hobj, NULL);
			T *filename = new T[len+1];
			if (!filename){
				goto jerror;
			}
			fnPDGetFileLinkName(hobj, filename);
			obj->filename = filename;
		} else
		if (objtype==PDCOBJ_OLE){
			if (!fnPDGetOleObjParam(hobj, obj->GetOleParamPtr())){
				goto jerror;
			}
			len = fnPDGetOleObjData(hobj, NULL, obj->GetAspectPtr());
			if (len==0){
				// error
				goto jerror;
			}
			unsigned char *data = new unsigned char[len];
			if (!data)
				goto jerror;
			fnPDGetOleObjData(hobj, data, obj->GetAspectPtr());
			obj->data = new TMemoryObject(data, len);
#if 0	// for debug
			{
				FILE *fp = fopen("d:\\temp\\ole.dat","wb");
				if (fp){
					fwrite(data, len, 1, fp);
					fclose(fp);
				}
			}
#endif
		} else
		if (objtype==PDCOBJ_EPWING){
			// get epwing data
			fnPDGetEPWingData(hobj, &obj->bookno, &obj->pos);
		} else
		if (objtype==PDCOBJ_BINARY){
			int len = fnPDGetLengthBinary(hobj);
			if (len==0){
				// Ignore this object.
				delete obj;
				continue;
			}
			uint8_t *data = new uint8_t[len];
			if (!data)
				goto jerror;
			memcpy(data, fnPDGetDataBinary(hobj), len);
			obj->data = (TMemoryObject*)new JLinkObject(data, len);	//TODO: cast‚¢‚ç‚È‚¢‚Í‚¸H
			obj->pos = fnPDGetExtraData(hobj);	// extra data
		}
		japa.objects.push_back(obj);
		continue;
jerror:
		delete obj;
		return -1;
	}
	return 0;
}
template <class T>
int DllDictionaryT<T>::SetObject(JapaT<T> &japa)
{
	int j = 0;
	for (int i=0;i<japa.getObjNum();i++){
		TObjDataT<T> &obj = *japa.objects[i];
		int objtype = obj.GetObjType();
		HPDOBJ pdobj = fnPDCreateObject(hPdc, objtype);
		if (!pdobj)
			continue;
		switch (objtype){
			case PDCOBJ_VOICE:
				break;
			case PDCOBJ_FILE:
				fnPDSetFileLinkName(pdobj, obj.GetFileName());
				break;
			case PDCOBJ_OLE:
				fnPDSetOleObj(pdobj, obj.GetOleParamPtr(), obj.GetData()->get(), obj.GetData()->getlen(), *obj.GetAspectPtr());
				break;
		}
		pdcdata.object[j] = pdobj;
		j++;
	}
	pdcdata.objnum = j;
	return j;
}

#endif

