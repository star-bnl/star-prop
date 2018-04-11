#ifndef StArray_cint_h
#define StArray_cint_h


#define StCollectionDef(QWERTY) \
class St ## QWERTY;\
typedef St ## QWERTY**            St ## QWERTY ## Iterator;\
typedef St ## QWERTY * const * const_St ## QWERTY ## Iterator;\
\
class StPtrVec ## QWERTY : public StRefArray \
{ \
public: \
StPtrVec ## QWERTY(Int_t sz=0):StRefArray(sz){};\
StPtrVec ## QWERTY(const StPtrVec ## QWERTY &from):StRefArray(from){};\
virtual        ~StPtrVec ## QWERTY(){};\
\
 St ## QWERTY * const &at(Int_t idx) const {return (St ## QWERTY  * const &)fV[idx];}\
 St ## QWERTY *       &at(Int_t idx)       {return (St ## QWERTY  *       &)fV[idx];}\
\
 St ## QWERTY * const &front()       const {return (St ## QWERTY  * const &)fV.front();}\
 St ## QWERTY *       &front()             {return (St ## QWERTY  *       &)fV.front();}\
\
 St ## QWERTY * const &back()        const {return (St ## QWERTY  * const &)fV.back();}\
 St ## QWERTY *       &back()              {return (St ## QWERTY  *       &)fV.back();}\
\
const_St ## QWERTY ## Iterator begin() const {return (const_St ## QWERTY ## Iterator)&(*(fV.begin()));}\
      St ## QWERTY ## Iterator begin()       {return (      St ## QWERTY ## Iterator)&(*(fV.begin()));}\
const_St ## QWERTY ## Iterator end()   const {return (const_St ## QWERTY ## Iterator)&(*(fV.end()));}\
      St ## QWERTY ## Iterator end()         {return (      St ## QWERTY ## Iterator)&(*(fV.end()));}\
      St ## QWERTY ## Iterator erase(St ## QWERTY ## Iterator  it)\
      {return (St ## QWERTY ## Iterator)Erase((TObject**)it,0);}\
      St ## QWERTY ## Iterator erase(St ## QWERTY ## Iterator fst,St ## QWERTY ## Iterator lst)\
      {return (St ## QWERTY ## Iterator)Erase((TObject**)fst,(TObject**)lst,0);}\
      St ## QWERTY *       &operator[](Int_t i)       {return at(i);}\
      St ## QWERTY * const &operator[](Int_t i) const {return at(i);}\
void  push_back(const St ## QWERTY * const to){fV.push_back((TObject*const)to);}\
\
ClassDef(StPtrVec ## QWERTY ,1) \
};\
\
\
class StSPtrVec ## QWERTY : public StStrArray \
{ \
public: \
StSPtrVec ## QWERTY(Int_t sz=0):StStrArray(sz){};\
StSPtrVec ## QWERTY(const StSPtrVec ## QWERTY &from):StStrArray(from){};\
\
 St ## QWERTY * const &at(Int_t idx) const {return (St ## QWERTY  * const &)fV[idx];}\
 St ## QWERTY *       &at(Int_t idx)       {return (St ## QWERTY  *       &)fV[idx];}\
\
 St ## QWERTY * const &front()       const {return (St ## QWERTY  * const &)fV.front();}\
 St ## QWERTY *       &front()             {return (St ## QWERTY  *       &)fV.front();}\
\
 St ## QWERTY * const &back()        const {return (St ## QWERTY  * const &)fV.back();}\
 St ## QWERTY *       &back()              {return (St ## QWERTY  *       &)fV.back();}\
\
const_St ## QWERTY ## Iterator begin() const {return (const_St ## QWERTY ## Iterator)&(*(fV.begin()));}\
      St ## QWERTY ## Iterator begin()       {return (      St ## QWERTY ## Iterator)&(*(fV.begin()));}\
const_St ## QWERTY ## Iterator end()   const {return (const_St ## QWERTY ## Iterator)&(*(fV.end()));}\
      St ## QWERTY ## Iterator end()         {return (      St ## QWERTY ## Iterator)&(*(fV.end()));}\
      St ## QWERTY ## Iterator erase(St ## QWERTY ## Iterator  it)\
      {return (St ## QWERTY ## Iterator)Erase((TObject**)it,1);}\
      St ## QWERTY ## Iterator erase(St ## QWERTY ## Iterator fst,St ## QWERTY ## Iterator lst)\
      {return (St ## QWERTY ## Iterator)Erase((TObject**)fst,(TObject**)lst,1);}\
      St ## QWERTY *       &operator[](Int_t i)       {return at(i);}\
      St ## QWERTY * const &operator[](Int_t i) const {return at(i);}\
void  push_back(const St ## QWERTY *to){StStrArray::push_back((TObject*)to);}\
\
ClassDef(StSPtrVec ## QWERTY,1) \
};\
typedef       St ## QWERTY ## Iterator  StPtrVec ## QWERTY ## Iterator;\
typedef const_St ## QWERTY ## Iterator  StPtrVec ## QWERTY ## ConstIterator;\
typedef       St ## QWERTY ## Iterator  StSPtrVec ## QWERTY ## Iterator;\
typedef const_St ## QWERTY ## Iterator  StSPtrVec ## QWERTY ## ConstIterator;\

//______________________________________________________________
#define StCollectionImp(QWERTY) \
\
ClassImpUnique(StPtrVec ## QWERTY ,1 ) \
void 		StPtrVec  ## QWERTY::Streamer(TBuffer& b){StRefArray::Streamer(b);} \
\
ClassImpUnique(StSPtrVec ## QWERTY , 2 ) \
\
void 		StSPtrVec ## QWERTY::Streamer(TBuffer& b){StStrArray::Streamer(b);} \


#endif
