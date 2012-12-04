#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "pref.hpp"
#include "pref_bison.h"
#include "pref_flex.h"
#include "utils.h"

int ltr_int_parser_parse(prefs *prf);
extern int ltr_int_parser_debug;

prefs *prefs::prf = NULL;
const char *parsed_file = NULL;
pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool parser_error = false;

prefs &prefs::getPrefs()
{
  if(pthread_mutex_lock(&sg_mutex) != 0){
    perror("mutex");
  }
  if(prf == NULL){
    prf = new prefs();
  }
  if(pthread_mutex_unlock(&sg_mutex) != 0){
    perror("mutex");
  }
  return *prf;
}

void prefs::freePrefs()
{
  if(pthread_mutex_lock(&sg_mutex) != 0){
    perror("mutex");
  }
  prefs *tmp = prf;
  prf = NULL;
  if(tmp != NULL){
    delete tmp;
  }
  if(pthread_mutex_unlock(&sg_mutex) != 0){
    perror("mutex");
  }
}

void ltr_int_parser_error(YYLTYPE *loc, prefs *prf, char const *s)
{
  (void) loc;
  (void) prf;
  ltr_int_log_message("%s in file %s, line %d near '%s'\n",
  		 s, parsed_file, ltr_int_parser_lineno, ltr_int_parser_text);
  parser_error = true;
}

  std::ostream &operator<<(std::ostream &stream, const sectionItem &s)
  {
    s.print(stream);
    return stream;
  }
  
  std::ostream &operator<<(std::ostream &stream, const prefItem &s)
  {
    s.print(stream);
    return stream;
  }

  void section::print(std::ostream &stream)const
  {
    stream<<"["<<*name<<"]"<<std::endl;
    std::vector<sectionItem*>::const_iterator i;
    for(i = items.begin(); i != items.end(); ++i){
      stream<<**i;
    }
    stream<<std::endl;
  }
  
  std::ostream &operator<<(std::ostream &stream, const prefs &q)
  {
    q.read_lock();
    std::vector<prefItem*>::const_iterator i;
    for(i = q.items.begin(); i != q.items.end(); ++i){
      stream<<**i<<std::endl;
    }
    q.unlock();
    stream<<std::endl;
    
    return stream;
  }

  section::~section()
  {
    delete name;
    for(size_t i = 0; i < items.size(); ++i) delete items[i];
  }

  prefs::~prefs()
  {
    write_lock();
    clear();
    unlock();
    pthread_rwlock_destroy(&lock);
  }
  
  bool prefs::changed()const
  {
    read_lock();
    bool res = changed_flag;
    unlock();
    return res;
  }
  
  void prefs::setChangeFlag()
  {
    changed_flag = true;
  }
  
  void prefs::resetChangeFlag()
  {
    write_lock();
    changed_flag = false;
    unlock();
  }

  void section::addItem(sectionItem *si)
  {
    items.push_back(si);
    si->registerItself(*this);
  }

  void prefs::addItem(prefItem *pi)
  {
    write_lock();
    setChangeFlag();
    items.push_back(pi);
    pi->registerItself(*this);
    unlock();
  }
  
  void keyVal::registerItself(section &parent)
  {
    parent.registerKeyVal(*this);
  }
  
  void section::registerKeyVal(keyVal &kv)
  {
    index.insert(std::pair<std::string, keyVal&>(kv.getKey(), kv));
  }
  
  void section::registerItself(prefs &parent)
  {
    parent.registerSection(*this);
  }
  
  //not locking - this function is not to be called from outside;
  //  only place to call this from is addItem...
  void prefs::registerSection(section &sec)
  {
    index.insert(std::pair<std::string, section&>(sec.getName(), sec));
  }
  
  bool prefs::getValue(const std::string &sec, const std::string &key, std::string &result)const
  {
    read_lock();
    std::map<std::string, section&>::const_iterator i = index.find(sec);
    if(i == index.end()){
      unlock();
      return false;
    }
    bool res = (i->second).getValue(key, result);
    unlock();
    return res;
  }

  bool section::getValue(const std::string &key, std::string &result)const
  {
    std::map<std::string, keyVal&>::const_iterator i = index.find(key);
    if(i == index.end()){
      return false;
    }
    result = (i->second).getValue();
    return true;
  }

  bool prefs::getValue(const std::string &sec, const std::string &key, float &result)const
  {
    std::string val;
    read_lock();
    if(!getValue(sec, key, val)){
      unlock();
      return false;
    }
    std::istringstream is(val);
    is >> result;
    unlock();
    return true;
  }
  
  bool prefs::getValue(const std::string &sec, const std::string &key, int &result)const
  {
    std::string val;
    read_lock();
    if(!getValue(sec, key, val)){
      unlock();
      return false;
    }
    std::istringstream is(val);
    is >> result;
    unlock();
    return true;
  }
  
  
  void prefs::getSectionList(std::vector<std::string> &sections)const
  {
    read_lock();
    std::map<std::string, section&>::const_iterator i;
    for(i = index.begin(); i != index.end(); ++i){
      sections.push_back((i->second).getName());
    }
    unlock();
  }
  
  bool prefs::sectionExists(const std::string &sec)const
  {
    read_lock();
    std::map<std::string, section&>::const_iterator i = index.find(sec);
    unlock();
    return (i != index.end());
  }
  
  bool section::keyExists(const std::string &key)const
  {
    std::map<std::string, keyVal&>::const_iterator i = index.find(key);
    return (i != index.end());
  }
  
  bool prefs::keyExists(const std::string &sec, const std::string &key)const
  {
    read_lock();
    std::map<std::string, section&>::const_iterator i = index.find(sec);
    if(i == index.end()){
      unlock();
      return false;
    }
    bool res = (i->second).keyExists(key);
    unlock();
    return res;
  }
  
  //Not locking here - addItem does all the actual changing...
  //  avoids deadlock...
  void prefs::addSection(const std::string &name)
  {
    if(sectionExists(name)){
      return;
    }
    section *s = new section();
    s->setName(name);
    addItem(s);
  }

  bool prefs::addSection(const std::string &nameTemplate, std::string &name)
  {
    std::ostringstream tmpName;
    size_t cntr = 0;
    
    while(1){
      tmpName.str(std::string());
      tmpName.clear();
      tmpName<<nameTemplate<<std::setfill('0')<<std::setw(4)<<cntr++;
      if(!sectionExists(tmpName.str())){
        name = tmpName.str();
        addSection(name);
        return true;
      }
      if(cntr > 9999){
        break;
      }
    }
    return false;
  }

  void section::addKey(const std::string &key, const std::string &value)
  {
    if(keyExists(key)){
      setValue(key, value);
      return;
    }
    keyVal *kv = new keyVal(new std::string(key), new std::string(value));
    addItem(kv);
  }
  
  void section::setValue(const std::string &key, const std::string &value)
  {
    if(!keyExists(key)){
      addKey(key, value);
      return;
    }
    std::map<std::string, keyVal&>::iterator i = index.find(key);
    (i->second).setValue(value);
  }
  
  void prefs::setValue(const std::string &sec, const std::string &key, const std::string &value)
  {
    if(!sectionExists(sec)){
      addSection(sec);
    }
    write_lock();
    setChangeFlag();
    std::map<std::string, section&>::iterator i = index.find(sec);
    (i->second).setValue(key, value);
    unlock();
  }
  
  void prefs::setValue(const std::string &sec, const std::string &key, const int &value)
  {
    if(!sectionExists(sec)){
      addSection(sec);
    }
    write_lock();
    setChangeFlag();
    std::map<std::string, section&>::iterator i = index.find(sec);
    std::ostringstream os;
    os<<value;
    (i->second).setValue(key, os.str());
    unlock();
  }
  
  void prefs::setValue(const std::string &sec, const std::string &key, const float &value)
  {
    if(!sectionExists(sec)){
      addSection(sec);
    }
    write_lock();
    setChangeFlag();
    std::map<std::string, section&>::iterator i = index.find(sec);
    std::ostringstream os;
    os<<value;
    (i->second).setValue(key, os.str());
    unlock();
  }
  
  void prefs::addKey(const std::string &sec, const std::string &key, const std::string &value)
  {
    if(!sectionExists(sec)){
      addSection(sec);
    }
    write_lock();
    setChangeFlag();
    std::map<std::string, section&>::iterator i = index.find(sec);
    (i->second).addKey(key, value);
    unlock();
  }
  
  bool prefs::findSection(const std::string &key, const std::string &value, std::string &name)
  {
    read_lock();
    std::map<std::string, section&>::const_iterator i;
    std::string val;
    for(i = index.begin(); i != index.end(); ++i){
      if(((i->second).getValue(key, val)) && (value.compare(val) == 0)){
        name = (i->second).getName();
        unlock();
        return true;
      }
    }
    unlock();
    return false;
  }
  
  void prefs::clear()
  {
    write_lock();
    changed_flag = false;
    for(size_t i = 0; i < items.size(); ++i){delete items[i];};
    items.clear();
    index.clear();
    unlock();
  }
  
/*****************************************************************************************/  
static bool parse_prefs(const std::string fname, prefs *prf)
{
  if((ltr_int_parser_in=fopen(fname.c_str(), "r")) != NULL){
    parsed_file = fname.c_str();
    //ltr_int_parser_debug=1;
    parser_error = false;
    int res = ltr_int_parser_parse(prf);
    fclose(ltr_int_parser_in);
    parsed_file = NULL;
    ltr_int_parser_lex_destroy();
    if((res == 0) && (!parser_error)){
      ltr_int_log_message("Preferences read OK!\n");
      prf->resetChangeFlag();
      return(true);
    }
  }
  ltr_int_log_message("Error encountered while reading preferences!\n");
  ltr_int_free_prefs();
  return(false);
}

bool ltr_int_read_prefs(const char *file, bool force_read)
{
  static bool prefs_ok = false;
  
//  if(force_read == true){
    prefs::getPrefs().clear();
    char *pfile;
    if(file != NULL){
      pfile = ltr_int_get_default_file_name(file);
    }else{
      pfile = ltr_int_get_default_file_name(NULL);
    }
    if(pfile == NULL){
      ltr_int_log_message("Can't identify file to read prefs from!\n");
      return false;
    }
    prefs_ok = parse_prefs(pfile, &(prefs::getPrefs()));
    free(pfile);
    if(prefs_ok){
      ltr_int_log_message("Dumping prefs:\n");
      ltr_int_dump_prefs("");
      ltr_int_log_message("================================================\n");
    }
//  }
  prefs::getPrefs().resetChangeFlag();
  return prefs_ok;
}

bool ltr_int_new_prefs(void)
{
  prefs::getPrefs().clear();
  return true;
}

void ltr_int_free_prefs(void)
{
  prefs::freePrefs();
}

bool ltr_int_dump_prefs(const std::string &file_name)
{
  if(file_name == ""){
    std::cerr<<prefs::getPrefs();
  }else{
    std::ofstream out(file_name.c_str());
    if(out.fail()){
      ltr_int_log_message("Can't open file '%s'!\n", file_name.c_str());
      return false;
    }
    out<<prefs::getPrefs();
    out.close();
  }
  return true;
}

bool ltr_int_need_saving(void)
{
  return prefs::getPrefs().changed();
}

bool ltr_int_save_prefs(const char *fname)
{
  std::string pfile;
  
  char *pfile_tmp = ((fname != NULL) ? ltr_int_my_strdup(fname) : ltr_int_get_default_file_name(NULL));
  if(pfile_tmp == NULL){
    ltr_int_log_message("Can't remember what the preference file name is!\n");
    return false;
  }
  pfile = std::string(pfile_tmp);
  free(pfile_tmp);
  
  std::string pfile_new(pfile + ".new");
  std::string pfile_old(pfile + ".old");
  
  if(!ltr_int_dump_prefs(pfile_new)){
    ltr_int_log_message("Can't store prefs to file '%s'!\n", pfile_new.c_str());
    return false;
  }
  prefs *check_prefs = new prefs();
  if(!parse_prefs(pfile_new, check_prefs)){
    delete check_prefs;
    ltr_int_log_message("Can't parse the new prefs back!\n");
    return false;
  }
  delete check_prefs;
  
  remove(pfile_old.c_str());
  if(rename(pfile.c_str(), pfile_old.c_str()) != 0){
    ltr_int_log_message("Can't rename '%s' to '%s'\n", pfile.c_str(), pfile_old.c_str());
  }
  if(rename(pfile_new.c_str(), pfile.c_str()) != 0){
    ltr_int_log_message("Can't rename '%s' to '%s'\n", pfile_new.c_str(), pfile.c_str());
    return false;
  }
  prefs::getPrefs().resetChangeFlag();
  return true;
}


void ltr_int_get_section_list(std::vector<std::string> &sections)
{
  prefs::getPrefs().getSectionList(sections);
}

char *ltr_int_get_key(const char *section_name, const char *key_name)
{
  std::string tmp;
  if(prefs::getPrefs().getValue(section_name, key_name, tmp)){
    return ltr_int_my_strdup(tmp.c_str());
  }else{
    return NULL;
  }
}

bool ltr_int_get_key_flt(const char *section_name, const char *key_name, float *val)
{
  return prefs::getPrefs().getValue(section_name, key_name, *val);
}

bool ltr_int_get_key_int(const char *section_name, const char *key_name, int *val)
{
  return prefs::getPrefs().getValue(section_name, key_name, *val);
}

bool ltr_int_change_key(const char *section_name, const char *key_name, const char *new_value)
{
  prefs::getPrefs().setValue(section_name, key_name, new_value);
  return true;
}

bool ltr_int_change_key_flt(const char *section_name, const char *key_name, float new_value)
{
  prefs::getPrefs().setValue(section_name, key_name, new_value);
  return true;
}

bool ltr_int_change_key_int(const char *section_name, const char *key_name, int new_value)
{
  prefs::getPrefs().setValue(section_name, key_name, new_value);
  return true;
}

char *ltr_int_find_section(const char *key_name, const char *value)
{
  std::string tmp;
  if(prefs::getPrefs().findSection(key_name, value, tmp)){
    return ltr_int_my_strdup(tmp.c_str());
  }
  return NULL;
}

char *ltr_int_add_unique_section(const char *name_template)
{
  std::string res;
  if(!prefs::getPrefs().addSection(name_template, res)){
    return NULL;
  }else{
    return ltr_int_my_strdup(res.c_str());
  }
}

///////////////////////////////////////////////////////////////////////////////
//                       Self contained unit test                            //
///////////////////////////////////////////////////////////////////////////////
#ifdef PREF_CPP_TEST
#include <cmath>
int tests = 0;
int fails = 0;


void printResult(bool success){
  ++tests;
  if(success){
    std::cout<<"OK";
  }else{
    ++fails;
    std::cout<<"FAILED!!!";
  }
  std::cout<<std::endl;
}


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  std::string secName1("Sekce");
  std::string secName2("Sekce2");
  std::string secName3("Zla Zla [oskliva] sekce3 ;)");
  std::string secName4("Sekce4");
  std::string itemKey1("str");
  std::string itemVal1("retezec");
  std::string itemKey2("int");
  int itemVal2 = 12345;
  std::string itemKey3("float");
  float itemVal3 = 3.1415926;
  float itemVal3_1 = 1.23456;
  const char *itemKey4 = "Title";
  const char *itemVal4 = "Titul";
  std::string itemVal4new = "Novy titul";
  std::string nameTemplate = "Game";
  //bool res;
  
  std::string prefFile("./prefs.txt");
  
  prefs &lprf = prefs::getPrefs();
  
  //Check that fresh prefs doesn't signalize change...
  std::cout<<"Checking change signalization inactive... ";
  printResult(!ltr_int_need_saving());
  
  ltr_int_change_key(secName2.c_str(), itemKey1.c_str(), itemVal1.c_str());
  ltr_int_change_key_int(secName1.c_str(), itemKey2.c_str(), itemVal2);
  ltr_int_change_key_flt(secName1.c_str(), itemKey3.c_str(), itemVal3);
  ltr_int_change_key_flt(secName4.c_str(), itemKey3.c_str(), itemVal3_1);
  lprf.addKey(secName1, itemKey4, itemVal4);
  
  std::cout<<"Checking for key nonexistence...";
  printResult(!lprf.keyExists(secName1, itemKey1));
  
  std::cout<<"Checking for key nonexistence part2...";
  printResult(!lprf.keyExists(secName3, itemKey1));

  std::cout<<"Checking for key existence...";
  printResult(lprf.keyExists(secName2, itemKey1));
  
  //Check that change is signalized
  std::cout<<"Checking change signalization active... ";
  printResult(ltr_int_need_saving());
  
  //Check storing of prefs...
  std::cout<<"Attempting to store prefs... ";
  printResult(ltr_int_save_prefs(prefFile.c_str()));
  
  //Check that saving prefs resets change flag...
  std::cout<<"Checking change signalization after pref save... ";
  printResult(!lprf.changed());
  
  std::cout<<"Clearing prefs... ";
  ltr_int_new_prefs();
  
  std::vector<std::string> sections;
  ltr_int_get_section_list(sections);
  printResult(sections.size() == 0);
  ltr_int_read_prefs(prefFile.c_str(), true);

  std::cout<<"Reading prefs back in...";
  printResult(ltr_int_read_prefs(prefFile.c_str(), true));
  
  std::cout<<"Checking if we read something in...";
  ltr_int_get_section_list(sections);
  printResult(sections.size() != 0);

  std::cout<<"Checking string pref...";
  std::string tmp_str;
  char *tmp_chr = ltr_int_get_key(secName2.c_str(), itemKey1.c_str());
  printResult((tmp_chr != NULL) && (tmp_chr == itemVal1));
  if(tmp_chr != NULL) free(tmp_chr);
  
  std::cout<<"Checking int pref...";
  int tmp_int;
  printResult(ltr_int_get_key_int(secName1.c_str(), itemKey2.c_str(), &tmp_int) 
              && (tmp_int == itemVal2));
  
  std::cout<<"Checking float pref...";
  float tmp_flt;
  printResult(ltr_int_get_key_flt(secName1.c_str(), itemKey3.c_str(), &tmp_flt) && 
              (fabsf(tmp_flt - itemVal3) < 1e-4));
  
  std::cout<<"Checking float pref part2...";
  printResult(ltr_int_get_key_flt(secName4.c_str(), itemKey3.c_str(), &tmp_flt) && 
              (fabsf(tmp_flt - itemVal3_1) < 1e-4));
  
  std::cout<<"Looking for section...";
  tmp_chr = ltr_int_find_section(itemKey4, itemVal4);
  printResult((tmp_chr != NULL) && (secName1 == tmp_chr));
  if(tmp_chr != NULL) free(tmp_chr);

  std::cout<<"Adding a key that already exists...";
  lprf.addKey(secName1, itemKey4, itemVal4new);
  printResult(lprf.getValue(secName1, itemKey4, tmp_str) && 
              (tmp_str == itemVal4new));
  
  std::cout<<"Looking for nonexistent section...";
  tmp_chr = ltr_int_find_section(itemKey4, itemVal4);
  printResult(tmp_chr == NULL);
  if(tmp_chr != NULL) free(tmp_chr);
  
  std::cout<<"Checking string value in nonexistent section...";
  tmp_chr = ltr_int_get_key(secName3.c_str(), itemKey1.c_str());
  printResult(tmp_chr == NULL);
  if(tmp_chr != NULL) free(tmp_chr);
  
  std::cout<<"Checking nonexistent string value...";
  tmp_chr = ltr_int_get_key(secName2.c_str(), itemKey2.c_str());
  printResult(tmp_chr == NULL);
  if(tmp_chr != NULL) free(tmp_chr);
  
  std::cout<<"Checking int value in nonexistent section...";
  printResult(!ltr_int_get_key_int(secName3.c_str(), itemKey2.c_str(), &tmp_int));
  
  std::cout<<"Checking flt value in nonexistent section...";
  printResult(!ltr_int_get_key_flt(secName3.c_str(), itemKey3.c_str(), &tmp_flt));
  
  lprf.addSection("Game0000");
  lprf.addSection("Game0001");
  lprf.addSection("Game0003");
  std::cout<<"Checking unique section creation (part1)...";
  printResult((!lprf.sectionExists("Game0002")) && (ltr_int_add_unique_section(nameTemplate.c_str()) != NULL) && (lprf.sectionExists("Game0002")));
  
  std::cout<<"Checking unique section creation (part2)...";
  printResult((!lprf.sectionExists("Game0004")) && (lprf.addSection(nameTemplate, tmp_str)) && (lprf.sectionExists("Game0004")));
  ltr_int_dump_prefs("");
  std::cout<<"Trying to dump prefs to a file out of reach...";
  printResult(!ltr_int_save_prefs("/shouldnt.work"));
  lprf.addKey(secName3, itemKey1, itemVal1);
  
  std::cout<<"Trying to dump corrupted prefs...";
  printResult(!ltr_int_save_prefs(prefFile.c_str()));
  
  ltr_int_free_prefs();
  
  std::cout<<"Trying to read linuxtrack prefs... (might fail)";
  printResult(ltr_int_read_prefs(NULL, true));
  
  std::cout<<tests<<" tests ran, "<<fails<<" failed..."<<std::endl;
  return 0;
}

#endif
