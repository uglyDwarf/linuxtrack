#ifndef PREF__HPP
#define PREF__HPP


  #include <string>
  #include <vector>
  #include <map>
  #include <iostream>
  
  #include <pthread.h>
  
  #include "pref.h"
  
  class section;
  class prefs;
  
  class sectionItem{
   public:
    sectionItem(){};
   friend std::ostream &operator<<(std::ostream &stream, const sectionItem &);
    virtual void print(std::ostream &stream) const = 0;
    virtual void registerItself(section &parent){(void) parent;};
    virtual ~sectionItem(){};
  };
  
  class keyVal : public sectionItem{
    std::string *key;
    std::string *value;
   public:
    keyVal(std::string *k, std::string *v):key(k), value(v){};
    virtual ~keyVal(){delete key; delete value;};
    virtual void print(std::ostream &stream) const{stream<<*key<<" = "<<*value<<std::endl;};
    std::string getKey()const{return std::string(*key);};    
    std::string getValue()const{return std::string(*value);};    
    virtual void registerItself(section &parent);
    void setValue(const std::string &val){*value = val;};
  };
  
  class secComment : public sectionItem{
    std::string *comm;
   public:
    secComment(std::string *c):comm(c){};
    virtual ~secComment(){delete comm;};
    virtual void print(std::ostream &stream) const{stream<<"###"<<*comm<<std::endl;};    
  };

  class prefItem{
   public:
    prefItem(){};
    friend std::ostream &operator<<(std::ostream &stream, const prefItem &s);    
    virtual void print(std::ostream &stream)const = 0;
    virtual ~prefItem(){}; 
    virtual void registerItself(prefs &parent){(void) parent;};
  };
  
  class section : public prefItem{
    std::string *name;
    std::vector<sectionItem*> items;
    std::map<std::string, keyVal*> index;
   public:
    section(){name = new std::string("");};
    virtual ~section();
    void setName(std::string *n){delete name; name = n;};
    void setName(const std::string &n){*name = n;};
    std::string getName()const{return std::string(*name);};
    friend std::ostream &operator<<(std::ostream &stream, const section &p); 
    void addItem(sectionItem *si);
    void registerKeyVal(keyVal &kv);
    virtual void print(std::ostream &stream)const;    
    virtual void registerItself(prefs &parent);
    bool getValue(const std::string &key, std::string &result)const;
    bool keyExists(const std::string &key)const;
    void addKey(const std::string &key, const std::string &value);
    void setValue(const std::string &key, const std::string &value);
  };
  
  class prefComment : public prefItem{
    std::string *comm;
   public:
    prefComment(std::string *c):comm(c){};
    virtual ~prefComment(){delete comm;};
    friend std::ostream &operator<<(std::ostream &stream, const prefComment &p);
    virtual void print(std::ostream &stream) const{stream<<"###"<<*comm<<std::endl;};    
  };
  
  class prefs{
    std::vector<prefItem*> items;
    std::map<std::string, section*> index;
    bool changed_flag;
    prefs();
    static prefs *prf;
    mutable pthread_rwlock_t lock;
    
    void read_lock()const{/*std::cout<<"R Lock"<<std::endl;*/pthread_rwlock_rdlock(&lock);};
    void write_lock()const{/*std::cout<<"W Lock"<<std::endl;*/pthread_rwlock_wrlock(&lock);};
    void unlock()const{/*std::cout<<"UnLock"<<std::endl;*/pthread_rwlock_unlock(&lock);};
   public:
    static prefs &getPrefs();
    static void freePrefs();
    ~prefs();
    friend std::ostream &operator<<(std::ostream &stream, const prefs &p);
    friend bool ltr_int_save_prefs(const char *fname);
    void addItem(prefItem *pi);
    void registerSection(section &sec);
    bool getValue(const std::string &sec, const std::string &key, std::string &result)const;
    bool getValue(const std::string &sec, const std::string &key, float &result)const;
    bool getValue(const std::string &sec, const std::string &key, int &result)const;
    void getSectionList(std::vector<std::string> &sections)const;
    bool sectionExists(const std::string &sec)const;
    bool keyExists(const std::string &sec, const std::string &key)const;
    void addSection(const std::string &name);
    bool addSection(const std::string &nameTemplate, std::string &name);
    void addKey(const std::string &sec, const std::string &key, const std::string &value);
    void setValue(const std::string &sec, const std::string &key, const std::string &value);
    void setValue(const std::string &sec, const std::string &key, const float &value);
    void setValue(const std::string &sec, const std::string &key, const int &value);
    bool findSection(const std::string &key, const std::string &value, std::string &name);
    bool findSections(const std::string &key, std::vector<std::string> &name);
    bool changed()const;
    void resetChangeFlag();
    void setChangeFlag();
    void clear();
  };

bool ltr_int_dump_prefs(const std::string &file_name);
#endif
