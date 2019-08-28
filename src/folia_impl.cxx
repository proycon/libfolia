/*
  Copyright (c) 2006 - 2019
  CLST  - Radboud University
  ILK   - Tilburg University

  This file is part of libfolia

  libfolia is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  libfolia is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      https://github.com/LanguageMachines/ticcutils/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl
*/

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <type_traits>
#include <stdexcept>
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/StringOps.h"
#include "ticcutils/XMLtools.h"
#include "ticcutils/Unicode.h"
#include "libfolia/folia.h"
#include "libfolia/folia_properties.h"
#include "config.h"

using namespace std;
using namespace icu;
using namespace TiCC;

namespace folia {
  using TiCC::operator <<;

  string VersionName() { return PACKAGE_STRING; }
  string Version() { return VERSION; }

  ElementType AbstractElement::element_id() const {
    return _props.ELEMENT_ID;
  }

  size_t AbstractElement::occurrences() const {
    return _props.OCCURRENCES;
  }

  size_t AbstractElement::occurrences_per_set() const {
    return _props.OCCURRENCES_PER_SET;
  }

  Attrib AbstractElement::required_attributes() const {
    return _props.REQUIRED_ATTRIBS;
  }

  Attrib AbstractElement::optional_attributes() const {
    return _props.OPTIONAL_ATTRIBS;
  }

  bool AbstractElement::hidden() const {
    return _props.HIDDEN;
  }

  map<const string, const string> reverse_old;

  const string& AbstractElement::xmltag() const {
    if ( reverse_old.empty() ){
      for ( const auto& it : oldtags ){
	reverse_old.insert( make_pair(it.second, it.first) );
      }
    }
    const string& result = _props.XMLTAG;
    if ( doc() && doc()->version_below(1,6) ){
      const auto& it = reverse_old.find(result);
      if ( it != reverse_old.end() ){
	return it->second;
      }
    }
    return result;
  }

  const string& AbstractElement::default_subset() const {
    return _props.SUBSET;
  }

  AnnotationType::AnnotationType AbstractElement::annotation_type() const {
    return _props.ANNOTATIONTYPE;
  }

  const set<ElementType>& AbstractElement::accepted_data() const {
    return _props.ACCEPTED_DATA;
  }

  const set<ElementType>& AbstractElement::required_data() const {
    return _props.REQUIRED_DATA;
  }

  bool AbstractElement::printable() const {
    return _props.PRINTABLE;
  }

  bool AbstractElement::speakable() const {
    return _props.SPEAKABLE;
  }

  bool AbstractElement::referable() const {
    return _props.WREFABLE;
  }

  bool AbstractElement::is_textcontainer() const {
    return _props.TEXTCONTAINER;
  }

  bool AbstractElement::is_phoncontainer() const {
    return _props.PHONCONTAINER;
  }

  bool AbstractElement::xlink() const {
    return _props.XLINK;
  }

  bool AbstractElement::auth() const {
    return _props.AUTH;
  }

  bool AbstractElement::setonly() const {
    return _props.SETONLY;
  }

  bool AbstractElement::auto_generate_id() const {
    return _props.AUTO_GENERATE_ID;
  }

  bool is_structure( const FoliaElement *el ){
    return dynamic_cast<const AbstractStructureElement*>( el ) != 0;
  }

  const string AbstractElement::href() const {
    auto it = _xlink.find("href");
    if ( it != _xlink.end() ){
      return it->second;
    }
    return "";
  }

  ostream& operator<<( ostream& os, const FoliaElement& ae ) {
    os << " <" << ae.classname();
    KWargs ats = ae.collectAttributes();
    if ( !ae.id().empty() ) {
      os << " xml:id='" << ae.id() << '"';
      ats.erase("xml:id");
    }

    for ( const auto& it: ats ) {
      os << " " << it.first << "='" << it.second << "'";
    }
    os << " > {";
    for ( size_t i=0; i < ae.size(); ++i ) {
      os << "<" << ae.index(i)->classname() << ">,";
    }
    os << "}";
    if ( ae.classname() == "t" ){
      os << " (" << ae.str() << ")";
    }
    return os;
  }

  ostream& operator<<( ostream&os, const FoliaElement *ae ) {
    if ( !ae ) {
      os << "nil";
    }
    else
      os << *ae;
    return os;
  }

  AbstractElement::AbstractElement( const properties& p, Document *d ) :
    _mydoc(d),
    _parent(0),
    _auth( p.AUTH ),
    _annotator_type(UNDEFINED),
    _refcount(0),
    _confidence(-1),
    _props(p)
  {
  }

  AbstractElement::~AbstractElement( ) {
    bool debug = false;
    if ( xmltag() == "w"
    	 || xmltag() == "s"
    	 || xmltag() == "entity"
    	 || xmltag() == "entities"
    	 || xmltag() == "morpheme"
    	 || xmltag() == "morphology" ){
      debug = false;
    }
    if (debug ){
      cerr << "delete " << xmltag() << " id=" << _id << " class= "
    	   << cls() << " datasize= " << _data.size() << endl;
      cerr << "REFCOUNT = " << refcount() << endl;
    }
    if ( refcount() > 0 ){
      if ( doc() ) {
	doc()->keepForDeletion( this );
      }
      //      decrefcount();
    }
    else {
      for ( const auto& el : _data ) {
	if ( el->refcount() == 0 ) {
	  if ( debug ){
	    cerr << "dus delete: " << el << endl;
	  }
	  // probably only != 0 for words
	  delete el;
	}
	else if ( doc() ) {
	  if ( debug ){
	    cerr << "dus KEEP: " << el << endl;
	  }
	  doc()->keepForDeletion( el );
	}
      }
    }
    if ( debug ){
      cerr << "\t\tsucces deleting element id=" << _id << " tag = " << xmltag() << " class= "
     	   << cls() << " datasize= " << _data.size() << endl;
    }
    if ( doc() ) {
      doc()->del_doc_index( this, _id );
      doc()->decrRef( annotation_type(), _set );
    }
  }

  xmlNs *AbstractElement::foliaNs() const {
    if ( doc() ) {
      return doc()->foliaNs();
    }
    return 0;
  }

  void AbstractElement::check_declaration(){
    if ( _mydoc ){
      string def;
      if ( !_set.empty() ){
	if ( !doc()->declared( annotation_type(), _set ) ) {
	  throw DeclarationError( "Set '" + _set
				  + "' is used but has no declaration " +
				  "for " + toString( annotation_type() )
				  + "-annotation" );
	}
      }
      else {
	if ( _mydoc->debug > 2 ) {
	  cerr << "get def for " <<  annotation_type() << endl;
	}
	def = doc()->default_set( annotation_type() );
	if ( doc()->debug > 2 ) {
	  cerr << "got def='" <<  def << "'" << endl;
	}
	if ( doc()->is_incremental() && def.empty() ){
	  // when there is NO default set, AND we are parsing using
	  // folia::Engine, we must check if there WAS an empty set originally
	  // which is 'obscured' by newly added declarations
	  def = doc()->original_default_set( annotation_type() );
	  if ( doc()->debug > 2 ) {
	    cerr << "from original got def='" <<  def << "'" << endl;
	  }
	}
	if ( !def.empty() ){
	  _set = def;
	}
	else if ( CLASS & required_attributes() ){
	  throw XmlError( "unable to assign a default set for tag: " + xmltag() );
	}
      }
      if ( annotation_type() != AnnotationType::NO_ANN
	   && !_mydoc->version_below( 2, 0 ) ){
	if ( !_mydoc->declared( annotation_type() ) ){
	  if ( _mydoc->autodeclare() ){
	    _mydoc->auto_declare( annotation_type(), _set );
	  }
	  else {
	    throw DeclarationError("1 Encountered an instance of <" + xmltag() + "> without a proper declaration" );
	  }
	}
	else if ( def.empty()
		  && !isSubClass( AbstractAnnotationLayer_t )
		  && !doc()->declared( annotation_type() ) ){
	  if ( _mydoc->autodeclare() ){
	    _mydoc->auto_declare( annotation_type(), _set );
	  }
	  else {
	    throw DeclarationError("2 Encountered an instance of <" + xmltag() + "> without a proper declaration" );
	  }
	}
	else if ( _set.empty()
		  && !isSubClass( AbstractAnnotationLayer_t )
		  && !doc()->declared( annotation_type(), "None" ) ){
	  if ( _mydoc->autodeclare() ){
	    _mydoc->auto_declare( annotation_type(), _set );
	  }
	  else {
	    throw DeclarationError("3 Encountered an instance of <" + xmltag() + "> without a proper declaration" );
	  }
	}
      }
    }
  }

  void AbstractElement::setAttributes( const KWargs& kwargs_in ) {
    KWargs kwargs = kwargs_in;
    Attrib supported = required_attributes() | optional_attributes();
    //#define LOG_SET_ATT
#ifdef LOG_SET_ATT
    if ( element_id() == Word_t ) {
      cerr << "set attributes: " << kwargs << " on " << classname() << endl;
      //      cerr << "required = " <<  toString(required_attributes()) << endl;
      //      cerr << "optional = " <<  optional_attributes() << endl;
      //cerr << "supported = " << supported << endl;
      //      cerr << "ID & supported = " << (ID & supported) << endl;
      //      cerr << "ID & _required = " << (ID & required_attributes() ) << endl;
      // cerr << "_id=" << _id << endl;
      // cerr << "AUTH : " << _auth << endl;
    }
#endif
    if ( doc() && doc()->debug > 2 ) {
      cerr << "set attributes: " << kwargs << " on " << classname() << endl;
    }

    string val = kwargs.extract( "generate_id" );
    if ( !val.empty() ) {
      if ( !doc() ) {
	throw runtime_error( "can't generate an ID without a doc" );
      }
      FoliaElement *e = (*doc())[val];
      if ( e ) {
	_id = e->generateId( xmltag() );
      }
      else {
	throw ValueError("Unable to generate an id from ID= " + val );
      }
    }
    else {
      val = kwargs.extract( "xml:id" );
      if ( val.empty() ) {
	val = kwargs.extract( "_id" ); // for backward compatibility
      }
      if ( !val.empty() ) {
	if ( (!ID) & supported ) {
	  throw ValueError("xml:id is not supported for " + classname() );
	}
	else if ( isNCName( val ) ){
	  _id = val;
	}
	else {
	  throw XmlError( "'" + val + "' is not a valid NCName." );
	}
      }
    }

    _set.clear();
    val = kwargs.extract( "set" );
    if ( !val.empty() ) {
      if ( !doc() ) {
	throw ValueError( "attribute set=" + val + " is used on a node without a document." );
      }
      if ( !( (CLASS & supported) || setonly() ) ) {
	throw ValueError("attribute 'set' is not supported for " + classname());
      }
      else {
	string st = doc()->unalias( annotation_type(), val );
	if ( st.empty() ){
	  _set = val;
	}
	else {
	  _set = st;
	}
      }
    }

    check_declaration();

    _class.clear();
    val = kwargs.extract( "class" );
    if ( !val.empty() ) {
      if ( !( CLASS & supported ) ) {
	throw ValueError("Class is not supported for " + classname() );
      }
      if ( element_id() != TextContent_t && element_id() != PhonContent_t ) {
	if ( !doc() ) {
	  throw ValueError( "Class=" + val + " is used on a node without a document." );
	}
	if ( _set.empty() ){
	  if ( !doc()->declared( annotation_type(), "None" ) ) {
	    cerr << endl << doc()->annotationdefaults() << endl << endl;
	    throw ValueError( xmltag() +": An empty set is used but that has no declaration "
			      "for " + toString( annotation_type() )
			      + "-annotation" );
	  }
	  _set = "None";
	}
	doc()->incrRef( annotation_type(), _set );
      }
      _class = val;
    }

    if ( element_id() != TextContent_t && element_id() != PhonContent_t ) {
      if ( !_class.empty() && _set.empty() ) {
	throw ValueError("Set is required for <" + classname() +
			 " class=\"" + _class + "\"> assigned without set."  );
      }
    }

    _annotator.clear();
    val = kwargs.extract( "annotator" );
    if ( !val.empty() ) {
      if ( !(ANNOTATOR & supported) ) {
	throw ValueError("attribute 'annotator' is not supported for " + classname() );
      }
      else {
	_annotator = val;
      }
    }
    else {
      string def;
      if ( doc() &&
	   (def = doc()->default_annotator( annotation_type(), _set )) != "" ) {
	_annotator = def;
      }
    }

    _annotator_type = UNDEFINED;
    val = kwargs.extract( "annotatortype" );
    if ( !val.empty() ) {
      if ( ! (ANNOTATOR & supported) ) {
	throw ValueError("Annotatortype is not supported for " + classname() );
      }
      else {
	_annotator_type = stringTo<AnnotatorType>( val );
	if ( _annotator_type == UNDEFINED ) {
	  throw ValueError( "annotatortype must be 'auto' or 'manual', got '"
			    + val + "'" );
	}
      }
    }
    else {
      if ( doc() ){
	AnnotatorType def = doc()->default_annotatortype( annotation_type(), _set );
	if ( def != UNDEFINED ) {
	  _annotator_type = def;
	}
      }
    }

    val = kwargs.extract( "processor" );
    if ( !val.empty() ){
      if ( _set.empty() ){
	_set = "None";
      }
      if ( !(ANNOTATOR & supported) ){
	throw ValueError("attribute 'processor' is not supported for " + classname() );
      }
      else {
	if ( doc() && doc()->get_processor(val) == 0 ){
	  throw ValueError("attribute 'processor' has unknown value: " + val );
	}
	if ( doc()
	     && !doc()->declared( annotation_type(), _set, "", _annotator_type, val ) ){
	  if (	!doc()->version_below( 2, 0 )
		&& doc()->autodeclare() ) {
	    KWargs args;
	    args["processor"] = val;
	    args["annotatortype"] = _annotator_type;
	    doc()->declare( annotation_type(), _set, args );
	  }
	  else {
	    throw DeclarationError( "Processor '" + val
				    + "' is used for annotationtype '"
				    + toString( annotation_type() )
				    + "' with set='" + _set +"'"
				    + " but there is no corresponding <annotator>"
				    + " referring to it in the annotation"
				    + " declaration block." );
	  }
	}
	_processor = val;
      }
    }
    else if ( (ANNOTATOR & supported) && doc() ){
      string def;
      try {
	def = doc()->default_processor( annotation_type(), _set );
      }
      catch ( const NoDefaultError& e ){
	if ( doc()->is_incremental() ){
	  // when there is NO default processor, AND we are parsing using
	  // folia::Engine, we must check if there WAS a processor originally
	  // which is 'obscured' by newly added declarations
	  def = doc()->original_default_processor( annotation_type() );
	  if ( doc()->debug > 2 ) {
	    cerr << "from original got default processor='" <<  def << "'" << endl;
	  }
	}
	else {
	  throw;
	}
      }
      _processor = def;
    }

    _confidence = -1;
    val = kwargs.extract( "confidence" );
    if ( !val.empty() ) {
      if ( !(CONFIDENCE & supported) ) {
	throw ValueError("Confidence is not supported for " + classname() );
      }
      else {
	try {
	  _confidence = stringTo<double>(val);
	  if ( _confidence < 0 || _confidence > 1.0 ){
	    throw ValueError("Confidence must be a floating point number between 0 and 1, got " + TiCC::toString(_confidence) );
	  }
	}
	catch (...) {
	  throw ValueError("invalid Confidence value, (not a number?)");
	}
      }
    }

    _n = "";
    val = kwargs.extract( "n" );
    if ( !val.empty() ) {
      if ( !(N & supported) ) {
	throw ValueError("N attribute is not supported for " + classname() );
      }
      else {
	_n = val;
      }
    }

    if ( xlink() ) {
      string type = "simple";
      val = kwargs.extract( "xlink:type" );
      if ( !val.empty() ) {
	type = val;
      }
      if ( type != "simple" && type != "locator" ) {
	throw XmlError( "only xlink:types: 'simple' and 'locator' are supported!" );
      }
      _xlink["type"] = type;
      val = kwargs.extract( "xlink:href" );
      if ( !val.empty() ) {
	_xlink["href"] = val;
      }
      else if ( type == "locator" ){
	throw XmlError( "xlink:type='locator' requires an 'xlink:href' attribute" );
      }
      val = kwargs.extract( "xlink:role" );
      if ( !val.empty() ) {
	_xlink["role"] = val;
      }
      val = kwargs.extract( "xlink:title" );
      if ( !val.empty() ) {
	_xlink["title"] = val;
      }
      val = kwargs.extract( "xlink:label" );
      if ( !val.empty() ) {
	if ( type == "simple" ){
	  throw XmlError( "xlink:type='simple' may not have an 'xlink:label' attribute" );
	}
	_xlink["label"] = val;
      }
      val = kwargs.extract( "xlink:arcrole" );
      if ( !val.empty() ) {
	if ( type == "locator" ){
	  throw XmlError( "xlink:type='locator' may not have an 'xlink:arcrole' attribute" );
	}
	_xlink["arcrole"] = val;
      }
      val = kwargs.extract( "xlink:show" );
      if ( !val.empty() ) {
	if ( type == "locator" ){
	  throw XmlError( "xlink:type='locator' may not have an 'xlink:show' attribute" );
	}
	_xlink["show"] = val;
      }
      val = kwargs.extract( "xlink:actuate" );
      if ( !val.empty() ) {
	if ( type == "locator" ){
	  throw XmlError( "xlink:type='locator' may not have an 'xlink:actuate' attribute" );
	}
	_xlink["actuate"] = val;
      }
    }

    _datetime.clear();
    val = kwargs.extract( "datetime" );
    if ( !val.empty() ) {
      if ( !(DATETIME & supported) ) {
	throw ValueError("datetime attribute is not supported for " + classname() );
      }
      else {
	string time = parseDate( val );
	if ( time.empty() ){
	  throw ValueError( "invalid datetime, must be in YYYY-MM-DDThh:mm:ss format: " + val );
	}
	_datetime = time;
      }
    }
    else {
      string def;
      if ( doc() &&
	   (def = doc()->default_datetime( annotation_type(), _set )) != "" ) {
	_datetime = def;
      }
    }
    val = kwargs.extract( "begintime" );
    if ( !val.empty() ) {
      if ( !(BEGINTIME & supported) ) {
	throw ValueError( "begintime attribute is not supported for " + classname() );
      }
      else {
	string time = parseTime( val );
	if ( time.empty() ) {
	  throw ValueError( "invalid begintime, must be in HH:MM:SS.mmm format: " + val );
	}
	_begintime = time;
      }
    }
    else {
      _begintime.clear();
    }
    val = kwargs.extract( "endtime" );
    if ( !val.empty() ) {
      if ( !(ENDTIME & supported) ) {
	throw ValueError( "endtime attribute is not supported for " + classname() );
      }
      else {
	string time = parseTime( val );
	if ( time.empty() ) {
	  throw ValueError( "invalid endtime, must be in HH:MM:SS.mmm format: " + val );
	}
	_endtime = time;
      }
    }
    else {
      _endtime.clear();
    }

    val = kwargs.extract( "src" );
    if ( !val.empty() ) {
      if ( !(SRC & supported) ) {
	throw ValueError( "src attribute is not supported for " + classname() );
      }
      else {
	_src = val;
      }
    }
    else {
      _src.clear();
    }

    if ( SPACE & supported ){
      _space = true;
    }
    val = kwargs.extract( "space" );
    if ( !val.empty() ) {
      if ( !(SPACE & supported) ){
	throw ValueError( "space attribute is not supported for " + classname() );
      }
      else {
	if ( val == "no" ) {
	  _space = false;
	}
	else if ( val == "yes" ) {
	  _space = true;
	}
	else {
	  throw ValueError( "invalid value for space attribute: '" + val + "'" );
	}
      }
    }

    val = kwargs.extract( "metadata" );
    if ( !val.empty() ) {
      if ( !(METADATA & supported) ) {
	throw ValueError( "Metadata attribute is not supported for " + classname() );
      }
      else {
	_metadata = val;
	if ( doc() && doc()->get_submetadata( _metadata ) == 0 ){
	  throw KeyError( "No such metadata defined: " + _metadata );
	}
      }
    }
    else
      _metadata.clear();

    val = kwargs.extract( "speaker" );
    if ( !val.empty() ) {
      if ( !(SPEAKER & supported) ) {
	throw ValueError( "speaker attibute is not supported for " + classname() );
      }
      else {
	_speaker = val;
      }
    }
    else {
      _speaker.clear();
    }

    val = kwargs.extract( "textclass" );
    if ( !val.empty() ) {
      if ( !(TEXTCLASS & supported) ) {
	throw ValueError( "textclass attribute is not supported for " + classname() );
      }
      else {
	_textclass = val;
      }
    }
    else {
      _textclass = "current";
    }

    val = kwargs.extract( "auth" );
    if ( !val.empty() ){
      _auth = stringTo<bool>( val );
    }
    if ( doc() && !_id.empty() ) {
      try {
	doc()->add_doc_index( this, _id );
      }
      catch ( const DuplicateIDError& e ){
	if ( element_id() != WordReference_t ){
	  throw;
	}
      }
    }
    addFeatureNodes( kwargs );
  }

  void AbstractElement::addFeatureNodes( const KWargs& kwargs ) {
    for ( const auto& it: kwargs ) {
      string tag = it.first;
      if ( tag == "head" ) {
	// "head" is special because the tag is "headfeature"
	// this to avoid conflicts with the "head" tag!
	tag = "headfeature";
      }
      if ( AttributeFeatures.find( tag ) == AttributeFeatures.end() ) {
	string message = "unsupported attribute: " + tag + "='" + it.second
	  + "' for node with tag '" + classname() + "'";
	if ( tag == "id" ){
	  message += "\ndid you mean xml:id?";
	}
	if ( doc() && doc()->permissive() ) {
	  cerr << message << endl;
	}
	else {
	  throw XmlError( message );
	}
      }
      KWargs newa;
      newa["class"] = it.second;
      FoliaElement *new_node = createElement( tag, doc() );
      new_node->setAttributes( newa );
      append( new_node );
    }
  }

  string toDoubleString( double d ){
    stringstream ss;
    ss.precision(6);
    ss << std::fixed << std::showpoint << d;
    return ss.str();
  }

  KWargs AbstractElement::collectAttributes() const {
    KWargs attribs;
    bool isDefaultSet = true;

    if ( !_id.empty() ) {
      attribs["xml:id"] = _id;
    }
    if ( _set != "None"
	 && !_set.empty()
	 && _set != doc()->default_set( annotation_type() ) ) {
      isDefaultSet = false;
      string ali = doc()->alias( annotation_type(), _set );
      if ( ali.empty() ){
	attribs["set"] = _set;
      }
      else {
	attribs["set"] = ali;
      }
    }
    if ( !_class.empty() ) {
      attribs["class"] = _class;
    }
    if ( !_processor.empty() ){
      string tmp;
      try {
	tmp = doc()->default_processor( annotation_type(), _set );
      }
      catch ( NoDefaultError& ){
      }
      catch ( ... ){
	throw;
      }
      if ( tmp != _processor ){
	attribs["processor"] = _processor;
      }
    }
    else {
      bool isDefaultAnn = true;
      if ( !_annotator.empty() &&
	   _annotator != doc()->default_annotator( annotation_type(), _set ) ) {
	isDefaultAnn = false;
	attribs["annotator"] = _annotator;
      }
      if ( _annotator_type != UNDEFINED ) {
	AnnotatorType at = doc()->default_annotatortype( annotation_type(), _set );
	if ( (!isDefaultSet || !isDefaultAnn) && _annotator_type != at ) {
	  if ( _annotator_type == AUTO ) {
	    attribs["annotatortype"] = "auto";
	  }
	  else if ( _annotator_type == MANUAL ) {
	    attribs["annotatortype"] = "manual";
	  }
	}
      }
    }
    if ( xlink() ) {
      auto it = _xlink.find("type");
      if ( it != _xlink.end() ){
	string type = it->second;
	if ( type == "simple" || type == "locator" ){
	  it = _xlink.find("href");
	  if ( it != _xlink.end() ){
	    attribs["xlink:href"] = it->second;
	    attribs["xlink:type"] = type;
	  }
	  it = _xlink.find("role");
	  if ( it != _xlink.end() ){
	    attribs["xlink:role"] = it->second;
	  }
	  it = _xlink.find("arcrole");
	  if ( it != _xlink.end() ){
	    attribs["xlink:arcrole"] = it->second;
	  }
	  it = _xlink.find("show");
	  if ( it != _xlink.end() ){
	    attribs["xlink:show"] = it->second;
	  }
	  it = _xlink.find("actuate");
	  if ( it != _xlink.end() ){
	    attribs["xlink:actuate"] = it->second;
	  }
	  it = _xlink.find("title");
	  if ( it != _xlink.end() ){
	    attribs["xlink:title"] = it->second;
	  }
	  it = _xlink.find("label");
	  if ( it != _xlink.end() ){
	    attribs["xlink:label"] = it->second;
	  }
	}
      }
    }
    if ( !_datetime.empty() &&
	 _datetime != doc()->default_datetime( annotation_type(), _set ) ) {
      attribs["datetime"] = _datetime;
    }
    if ( !_begintime.empty() ) {
      attribs["begintime"] = _begintime;
    }
    if ( !_endtime.empty() ) {
      attribs["endtime"] = _endtime;
    }
    if ( !_src.empty() ) {
      attribs["src"] = _src;
    }
    if ( !_metadata.empty() ) {
      attribs["metadata"] = _metadata;
    }
    if ( !_speaker.empty() ) {
      attribs["speaker"] = _speaker;
    }
    if ( !_textclass.empty() && _textclass != "current" ){
      attribs["textclass"] = _textclass;
    }

    if ( _confidence >= 0 ) {
      attribs["confidence"] = toDoubleString(_confidence);
    }
    if ( !_n.empty() ) {
      attribs["n"] = _n;
    }
    if ( !_auth ) {
      attribs["auth"] = "no";
    }
    if ( SPACE & optional_attributes() ){
      if ( !_space ) {
	attribs["space"] = "no";
      }
    }
    return attribs;
  }

  const string FoliaElement::xmlstring( bool add_ns ) const{
    // serialize to a string (XML fragment)
    xmlNode *n = xml( true, false );
    if ( add_ns ){
      xmlSetNs( n, xmlNewNs( n, (const xmlChar *)NSFOLIA.c_str(), 0 ) );
    }
    xmlBuffer *buf = xmlBufferCreate();
    xmlNodeDump( buf, 0, n, 0, 0 );
    string result = (const char*)xmlBufferContent( buf );
    xmlBufferFree( buf );
    xmlFreeNode( n );
    return result;
  }

  const string FoliaElement::xmlstring( bool format,
					int indent,
					bool add_ns ) const{
    // serialize to a string (XML fragment)
    xmlNode *n = xml( true, false );
    if ( add_ns ){
      xmlSetNs( n, xmlNewNs( n, (const xmlChar *)NSFOLIA.c_str(), 0 ) );
    }
    xmlBuffer *buf = xmlBufferCreate();
    //    xmlKeepBlanksDefault(0);
    xmlNodeDump( buf, 0, n, indent, (format?1:0) );
    string result = (const char*)xmlBufferContent( buf );
    xmlBufferFree( buf );
    xmlFreeNode( n );
    return result;
  }

  string tagToAtt( const FoliaElement* c ) {
    string att;
    if ( c->isSubClass( Feature_t ) ) {
      att = c->xmltag();
      if ( att == "feat" ) {
	// "feat" is a Feature_t too. exclude!
	att = "";
      }
      else if ( att == "headfeature" ) {
	// "head" is special
	att = "head";
      }
    }
    return att;
  }

  void AbstractElement::check_append_text_consistency( const FoliaElement *child ) const {
    if ( !doc() || !doc()->checktext() ){
      return;
    }
    string cls = child->cls();
    if ( !child->hastext( cls ) ){
      // no use to proceed. not adding text
      return;
    }
    FoliaElement *parent = this->parent();
    if ( parent
	 && parent->element_id() != Correction_t
	 && parent->hastext( cls ) ){
      // check text consistency for parents with text
      // but SKIP Corrections
      UnicodeString s1 = parent->text( cls, TEXT_FLAGS::STRICT );
      UnicodeString s2 = child->text( cls, TEXT_FLAGS::STRICT );
      // no retain tokenization, strict for both
      s1 = normalize_spaces( s1 );
      s2 = normalize_spaces( s2 );
      if ( !s1.isEmpty() && !s2.isEmpty() ){
	bool test_fail;
	if ( isSubClass( Word_t )
	     || child->isSubClass( TextContent_t )
	     || isSubClass( String_t ) ){
	  // Words and Strings are 'per definition' PART of there parents
	  test_fail = ( s1.indexOf( s2 ) < 0 ); // aren't they?
	}
	else {
	  // otherwise an exacte match is needed
	  test_fail = ( s1 != s2 );
	}
	if ( test_fail ){
	  throw InconsistentText( "text (class="
				  + cls + ") from node: " + child->xmltag()
				  + "(" + child->id() + ")"
				  + " with value\n'" + TiCC::UnicodeToUTF8(s2)
				  + "'\n to element: " + parent->xmltag() +
				  + "(" + parent->id() + ") which already has "
				  + "text in that class and value: \n'"
				  + TiCC::UnicodeToUTF8(s1) + "'\n" );
	}
      }
    }
  }

  void AbstractElement::check_text_consistency( ) const {
    if ( !doc() || !doc()->checktext() || ! printable() ){
      return;
    }
    // check if the text associated with all children is compatible with the
    // parents parental text.

    string cls = this->cls();
    FoliaElement *parent = this->parent();
    if ( parent
	 && parent->element_id() != Correction_t
	 && parent->hastext( cls ) ){
      // check text consistency for parents with text
      // but SKIP Corrections
      UnicodeString s1 = parent->text( cls, TEXT_FLAGS::STRICT );
      UnicodeString s2 = this->text( cls, TEXT_FLAGS::NONE );
      // no retain tokenization, strict for parent, deeper for child
      s1 = normalize_spaces( s1 );
      s2 = normalize_spaces( s2 );
      bool test_fail;
      if ( isSubClass( Word_t )
	   || isSubClass( String_t ) ) {
	// Words and Strings are 'per definition' PART of there parents
	test_fail = ( s1.indexOf( s2 ) < 0 ); // aren't they?
      }
      else {
	// otherwise an exacte match is needed
	test_fail = ( s1 != s2 );
      }
      if ( test_fail ){
	throw InconsistentText( "text (class="
				+ cls + ") from node: " + xmltag()
				+ "(" + id() + ")"
				+ " with value\n'" + TiCC::UnicodeToUTF8(s2)
				+ "'n to element: " + parent->xmltag() +
				+ "(" + parent->id() + ") which already has "
				+ "text in that class and value: \n'"
				+ TiCC::UnicodeToUTF8(s1) + "'\n" );
      }
    }
  }

  xmlNode *AbstractElement::xml( bool recursive, bool kanon ) const {
    xmlNode *e = XmlNewNode( foliaNs(), xmltag() );
    KWargs attribs = collectAttributes();
    set<FoliaElement *> attribute_elements;
    // nodes that can be represented as attributes are converted to atributes
    // and excluded of 'normal' output.

    map<string,int> af_map;
    // first we search al features that can be serialized to an attribute
    // and count them!
    for ( const auto& el : _data ) {
      string at = tagToAtt( el );
      if ( !at.empty() ) {
	++af_map[at];
      }
    }
    // ok, now we create attributes for those that only occur once
    for ( const auto& el : _data ) {
      string at = tagToAtt( el );
      if ( !at.empty() && af_map[at] == 1 ) {
	attribs[at] = el->cls();
	attribute_elements.insert( el );
      }
    }
    addAttributes( e, attribs );
    if ( recursive ) {
      // append children:
      // we want make sure that text elements are in the right order,
      // in front and the 'current' class first
      list<FoliaElement *> currenttextelements;
      list<FoliaElement *> textelements;
      list<FoliaElement *> otherelements;
      list<FoliaElement *> commentelements;
      multimap<ElementType, FoliaElement *, std::greater<ElementType>> otherelementsMap;
      for ( const auto& el : _data ) {
	if ( attribute_elements.find(el) == attribute_elements.end() ) {
	  if ( el->isinstance(TextContent_t) ) {
	    if ( el->cls() == "current" ) {
	      currenttextelements.push_back( el );
	    }
	    else {
	      textelements.push_back( el );
	    }
	  }
	  else {
	    if ( kanon ) {
	      otherelementsMap.insert( make_pair( el->element_id(), el ) );
	    }
	    else {
	      if ( el->isinstance(XmlComment_t)
		   && currenttextelements.empty()
		   && textelements.empty() ) {
		commentelements.push_back( el );
	      }
	      else {
		otherelements.push_back( el );
	      }
	    }
	  }
	}
      }
      for ( const auto& cel : commentelements ) {
	xmlAddChild( e, cel->xml( recursive, kanon ) );
      }
      for ( const auto& tel : currenttextelements ) {
	xmlAddChild( e, tel->xml( recursive, false ) );
	// don't change the internal sequences of TextContent elements
      }
      for ( const auto& tel : textelements ) {
	xmlAddChild( e, tel->xml( recursive, false ) );
	// don't change the internal sequences of TextContent elements
      }
      if ( !kanon ) {
	for ( const auto& oel : otherelements ) {
	  xmlAddChild( e, oel->xml( recursive, kanon ) );
	}
      }
      else {
	for ( const auto& oem : otherelementsMap ) {
	  xmlAddChild( e, oem.second->xml( recursive, kanon ) );
	}
      }
      check_text_consistency();
    }
    return e;
  }

  const string AbstractElement::str( const string& cls ) const {
    // if this is a TextContent or it may contain TextContent
    // then return the associated text()
    // if this is a PhonContent or it may contain PhonContent
    // then return the associated phon()
    // otherwise return empty string
    UnicodeString us;
    try {
      us = text(cls);
    }
    catch( const NoSuchText& ){
      try {
	us = phon(cls);
      }
      catch( const NoSuchPhon&){
	// No TextContent or Phone allowed
      }
    }
    return TiCC::UnicodeToUTF8( us );
  }

  const string AbstractElement::speech_src() const {
    if ( !_src.empty() ) {
      return _src;
    }
    if ( _parent ) {
      return _parent->speech_src();
    }
    return "";
  }

  const string AbstractElement::speech_speaker() const {
    if ( !_speaker.empty() ) {
      return _speaker;
    }
    if ( _parent ) {
      return _parent->speech_speaker();
    }
    return "";
  }

  const string AbstractElement::language( const string& st ) const {
    std::set<ElementType> exclude;
    vector<LangAnnotation*> v = select<LangAnnotation>( st, exclude, false );
    if ( v.size() > 0 ){
      return v[0]->cls();
    }
    else if ( _parent ){
      return _parent->language( st );
    }
    else {
      return doc()->language();
    }
  }

  bool FoliaElement::hastext( const string& cls ) const {
    // does this element have a TextContent with class 'cls'
    // Default is class="current"
    try {
      this->text_content(cls);
      return true;
    } catch ( const NoSuchText& e ) {
      return false;
    }
  }

  bool FoliaElement::hasphon( const string& cls ) const {
    // does this element have a TextContent with class 'cls'
    // Default is class="current"
    try {
      this->phon_content(cls);
      return true;
    } catch ( const NoSuchPhon& e ) {
      return false;
    }
  }

  //#define DEBUG_TEXT
  //#define DEBUG_TEXT_DEL

  const string SPACE_STRING = " ";

  const string& AbstractElement::get_delimiter( bool retaintok ) const {
#ifdef DEBUG_TEXT_DEL
    cerr << "IN <" << xmltag() << ">:get_delimiter (" << retaintok << ")" << endl;
#endif
    if ( _props.TEXTDELIMITER != "NONE" ) {
      return _props.TEXTDELIMITER;
    }
    else if ( _data.size() > 0 ) {
      // attempt to get a delimiter from the last child
      FoliaElement *last = _data.back();
      if ( last->isSubClass(AbstractStructureElement_t) ){
	const string& det = last->get_delimiter( retaintok );
#ifdef DEBUG_TEXT_DEL
	cerr << "out <" << xmltag() << ">:get_delimiter ==> '" << det << "'" << endl;
#endif
	return det;
      }
    }
    if ( (SPACE & optional_attributes()) ){
      if ( _space || retaintok ){
#ifdef DEBUG_TEXT_DEL
	cerr << "out <" << xmltag() << ">:get_delimiter ==> ' '" << endl;
#endif
	return SPACE_STRING;
      }
    }
#ifdef DEBUG_TEXT_DEL
    cerr << "out <" << xmltag() << ">:get_delimiter ==> ''" << endl;
#endif
    return EMPTY_STRING;
  }

  const UnicodeString AbstractElement::private_text( const string& cls,
						     bool retaintok,
						     bool strict,
						     bool show_hidden ) const {
    // get the UnicodeString value of underlying elements
    // default cls="current"
#ifdef DEBUG_TEXT
    cerr << "TEXT(" << cls << ") op node : " << xmltag() << " id ( " << id() << ")" << endl;
#endif
    if ( strict ) {
      return text_content(cls,show_hidden)->text();
    }
    else if ( is_textcontainer() ){
      UnicodeString result;
      for ( const auto& d : _data ){
	if ( d->printable() ){
	  if ( !result.isEmpty() ){
	    const string& delim = d->get_delimiter( retaintok );
	    result += TiCC::UnicodeFromUTF8(delim);
	  }
	  result += d->text( cls );
	}
      }
#ifdef DEBUG_TEXT
      cerr << "TEXT op a textcontainer :" << xmltag() << " returned '" << result << "'" << endl;
#endif
      return result;
    }
    else if ( !printable() || ( hidden() && !show_hidden ) ){
      throw NoSuchText( "NON printable element: " + xmltag() );
    }
    else {
      TEXT_FLAGS flags = TEXT_FLAGS::NONE;
      if ( retaintok ){
	flags |= TEXT_FLAGS::RETAIN;
      }
      if ( show_hidden ){
	flags |= TEXT_FLAGS::HIDDEN;
      }
      UnicodeString result = deeptext( cls, flags );
      if ( result.isEmpty() ) {
	result = stricttext( cls );
      }
      if ( result.isEmpty() ) {
	throw NoSuchText( "on tag " + xmltag() + " nor it's children" );
      }
      return result;
    }
  }

  const UnicodeString AbstractElement::text( const std::string& st,
					     TEXT_FLAGS flags ) const {
    bool retain = ( TEXT_FLAGS::RETAIN & flags ) == TEXT_FLAGS::RETAIN;
    bool strict = ( TEXT_FLAGS::STRICT & flags ) == TEXT_FLAGS::STRICT;
    bool hidden = ( TEXT_FLAGS::HIDDEN & flags ) == TEXT_FLAGS::HIDDEN;
    return private_text( st, retain, strict, hidden );
  }

  void FoLiA::setAttributes( const KWargs& args ){
    KWargs atts = args;
    // we store some attributes in the document itself
    doc()->setDocumentProps( atts );
    // use remaining attributes for the FoLiA node
    // probably only the ID
    AbstractElement::setAttributes( atts );
  }

  FoliaElement* FoLiA::parseXml( const xmlNode *node ){
    ///
    /// recursively parse a complete FoLiA tree from @node
    /// the topnode is special, as it carries the main document properties
    ///
    KWargs atts = getAttributes( node );
    if ( !doc() ){
      throw XmlError( "FoLiA root without Document" );
    }
    setAttributes( atts );
    bool meta_found = false;
    xmlNode *p = node->children;
    while ( p ){
      if ( p->type == XML_ELEMENT_NODE ){
	if ( TiCC::Name(p) == "metadata" &&
	     checkNS( p, NSFOLIA ) ){
	  if ( doc()->debug > 1 ){
	    cerr << "Found metadata" << endl;
	  }
	  doc()->parse_metadata( p );
	  meta_found = true;
	}
	else if ( p && TiCC::getNS(p) == NSFOLIA ){
	  string tag = TiCC::Name( p );
	  if ( !meta_found  && !doc()->version_below(1,6) ){
	    throw XmlError( "Expecting element metadata, got '" + tag + "'" );
	  }
	  FoliaElement *t = AbstractElement::createElement( tag, doc() );
	  if ( t ){
	    if ( doc()->debug > 2 ){
	      cerr << "created " << t << endl;
	    }
	    t = t->parseXml( p );
	    if ( t ){
	      if ( doc()->debug > 2 ){
		cerr << "extend " << this << " met " << tag << endl;
	      }
	      this->append( t );
	    }
	  }
	}
      }
      p = p->next;
    }
    return this;
  }

  const UnicodeString FoLiA::private_text( const string& cls,
					   bool retaintok,
					   bool strict,
					   bool ) const {
#ifdef DEBUG_TEXT
    cerr << "FoLiA::TEXT(" << cls << ")" << endl;
#endif
    const vector<FoliaElement *>& data = this->data();
    UnicodeString result;
    for ( const auto& d : data ){
      if ( !result.isEmpty() ){
	const string& delim = d->get_delimiter( retaintok );
	result += TiCC::UnicodeFromUTF8(delim);
      }
      result += d->private_text( cls, retaintok, strict, false );
    }
#ifdef DEBUG_TEXT
    cerr << "FoLiA::TEXT returns '" << result << "'" << endl;
#endif
    return result;
  }

  UnicodeString trim_space( const UnicodeString& in ){
    UnicodeString cmp = " ";
    //    cerr << "in = '" << in << "'" << endl;
    UnicodeString out;
    int i = 0;
    for( ; i < in.length(); ++i ){
      //      cerr << "start: bekijk:" << UnicodeString(in[i]) << endl;
      if ( in[i] != cmp[0] ){
	break;
      }
    }
    int j = in.length()-1;
    for( ; j >= 0; --j ){
      //      cerr << "end: bekijk:" << UnicodeString(in[j]) << endl;
      if ( in[j] != cmp[0] ){
	break;
      }
    }
    // cerr << "I=" << i << endl;
    // cerr << "J=" << j << endl;
    if ( j < i ){
      //      cerr << "out = LEEG" << endl;
      return out;
    }
    out = UnicodeString( in, i, j-i+1 );
    //    cerr << "out = '" << out << "'" << endl;
    return out;
  }

  bool check_end( const UnicodeString& us, bool& only ){
    only = false;
    string tmp = TiCC::UnicodeToUTF8( us );
    int j = tmp.length()-1;
    size_t found_nl = 0;
    for ( ; j >=0; --j ){
      if ( tmp[j] == '\n' ){
	++found_nl;
      }
      else {
	break;
      }
    }
    only = found_nl == tmp.length();
    return found_nl > 0;
  }

  bool no_space_at_end( FoliaElement *s ){
    bool result = false;
    //    cerr << "no space? s: " << s << endl;
    if ( s ){
      vector<Word*> words = s->select<Word>(false);
      if ( !words.empty() ){
	Word *last = words.back();
	//	cerr << "no space? last: " << last << endl;
	return !last->space();
      }
    }
    return result;
  }

  const UnicodeString AbstractElement::deeptext( const string& cls,
						 TEXT_FLAGS flags ) const {
    // get the UnicodeString value of underlying elements
    // default cls="current"
#ifdef DEBUG_TEXT
    cerr << "deepTEXT(" << cls << ") op node : " << xmltag() << " id(" << id() << ") cls=" << this->cls() << ")" << endl;
#endif
#ifdef DEBUG_TEXT
    cerr << "deeptext: node has " << _data.size() << " children." << endl;
#endif
    vector<UnicodeString> parts;
    vector<UnicodeString> seps;
    for ( const auto& child : this->data() ) {
      // try to get text dynamically from children
      // skip TextContent elements
#ifdef DEBUG_TEXT
      if ( !child->printable() ) {
	cerr << "deeptext: node[" << child->xmltag() << "] NOT PRINTABLE! " << endl;
      }
#endif
      if ( child->printable()
	   && ( is_structure( child )
		|| child->isSubClass( AbstractSpanAnnotation_t )
		|| child->isinstance( Correction_t ) )
	   && !child->isinstance( TextContent_t ) ) {
#ifdef DEBUG_TEXT
	cerr << "deeptext:bekijk node[" << child->xmltag() << "]"<< endl;
#endif
	try {
	  UnicodeString tmp = child->text( cls, flags );
#ifdef DEBUG_TEXT
	  cerr << "deeptext found '" << tmp << "'" << endl;
#endif
	  if ( !isSubClass(AbstractTextMarkup_t) ){
	    //	    tmp.trim();
	    tmp = trim_space( tmp );
	  }
#ifdef DEBUG_TEXT
	  cerr << "deeptext trimmed '" << tmp << "'" << endl;
#endif
	  parts.push_back(tmp);
	  if ( child->isinstance( Sentence_t )
	       && no_space_at_end(child) ){
	    const string& delim = "";
#ifdef DEBUG_TEXT
	    cerr << "deeptext: no delimiter van "<< child->xmltag() << " on"
		 << " last w of s" << endl;
#endif
	    seps.push_back(TiCC::UnicodeFromUTF8(delim));
	  }
	  else {
	    // get the delimiter
	    bool retain = ( TEXT_FLAGS::RETAIN & flags ) == TEXT_FLAGS::RETAIN;
	    const string& delim = child->get_delimiter( retain );
#ifdef DEBUG_TEXT
	    cerr << "deeptext:delimiter van "<< child->xmltag() << " ='" << delim << "'" << endl;
#endif
	    seps.push_back(TiCC::UnicodeFromUTF8(delim));
	  }
	} catch ( const NoSuchText& e ) {
#ifdef DEBUG_TEXT
	  cerr << "HELAAS" << endl;
#endif
	}
      }
    }

    // now construct the result;
    UnicodeString result;
    for ( size_t i=0; i < parts.size(); ++i ) {
#ifdef DEBUG_TEXT
      cerr << "part[" << i << "]='" << parts[i] << "'" << endl;
      cerr << "sep[" << i << "]='" << seps[i] << "'" << endl;
#endif
      bool only_nl = false;
      bool end_is_nl = check_end( parts[i], only_nl );
      if ( end_is_nl ){
#ifdef DEBUG_TEXT
	cerr << "a newline after: '" << parts[i] << "'" << endl;
	if ( i < parts.size()-1 ){
	  cerr << "next sep='" << seps[i+1] << "'" << endl;
	}
#endif

	if ( only_nl ){
	  // only a newline
	  result = trim_space( result );
#ifdef DEBUG_TEXT
	  cerr << "OK it is only newline(s)" << endl;
	  cerr << "TRIMMED? '" << result << "'" << endl;
#endif
	}
      }
      result += parts[i];
      if ( !end_is_nl && i < parts.size()-1 ){
	result += seps[i];
      }
#ifdef DEBUG_TEXT
      cerr << "result='" << result << "'" << endl;
#endif
    }
#ifdef DEBUG_TEXT
    cerr << "deeptext() for " << xmltag() << " step 3 " << endl;
#endif
    if ( result.isEmpty() ) {
      bool hidden = ( TEXT_FLAGS::HIDDEN & flags ) == TEXT_FLAGS::HIDDEN;
      result = text_content(cls,hidden)->text();
    }
#ifdef DEBUG_TEXT
    cerr << "deeptext() for " << xmltag() << " result= '" << result << "'" << endl;
#endif
    if ( result.isEmpty() ) {
      throw NoSuchText( xmltag() + ":(class=" + cls +"): empty!" );
    }
    return result;
  }

  const UnicodeString FoliaElement::stricttext( const string& cls ) const {
    // get UnicodeString content of TextContent children only
    // default cls="current"
    return this->text(cls, TEXT_FLAGS::STRICT );
  }

  const UnicodeString FoliaElement::toktext( const string& cls ) const {
    // get UnicodeString content of TextContent children only
    // default cls="current"
    return this->text(cls, TEXT_FLAGS::RETAIN );
  }

  const TextContent *AbstractElement::text_content( const string& cls,
						    bool show_hidden ) const {
    // Get the text explicitly associated with this element
    // (of the specified class) the default class is 'current'
    // Returns the TextContent instance rather than the actual text.
    // (so it might return iself.. ;)
    // Does not recurse into children
    // with sole exception of Correction
    // Raises NoSuchText exception if not found.
#ifdef DEBUG_TEXT
    cerr << "text_content(" << cls << "," << (show_hidden?"show_hidden":"")
	 << ")" << endl;
#endif
    if ( isinstance(TextContent_t) ){
#ifdef DEBUG_TEXT
      cerr << "A textcontent!!" << endl;
#endif
      if  ( this->cls() == cls ) {
	return dynamic_cast<const TextContent*>(this);
      }
      else {
	throw NoSuchText( "TextContent::text_content(" + cls + ")" );
      }
    }
#ifdef DEBUG_TEXT
    cerr << (!printable()?"NOT":"") << " printable: " << xmltag() << endl;
    cerr << (!hidden()?"NOT":"") << " hidden: " << xmltag() << endl;
#endif
    if ( !printable() || ( hidden() && !show_hidden ) ) {
      throw NoSuchText( "non-printable element: " +  xmltag() );
    }
    for ( const auto& el : data() ) {
      if ( el->isinstance(TextContent_t) && (el->cls() == cls) ) {
	return dynamic_cast<TextContent*>(el);
      }
      else if ( el->element_id() == Correction_t) {
	try {
	  return el->text_content(cls,show_hidden);
	} catch ( const NoSuchText& e ) {
	  // continue search for other Corrections or a TextContent
	}
      }
    }
    throw NoSuchText( xmltag() + "::text_content(" + cls + ")" );
  }

  const PhonContent *AbstractElement::phon_content( const string& cls,
						    bool show_hidden ) const {
    // Get the phon explicitly associated with this element
    // (of the specified class) the default class is 'current'
    // Returns the PhonContent instance rather than the actual phoneme.
    // Does not recurse into children
    // with sole exception of Correction
    // Raises NoSuchPhon exception if not found.
    if ( isinstance(PhonContent_t) ){
      if  ( this->cls() == cls ) {
	return dynamic_cast<const PhonContent*>(this);
      }
      else {
	throw NoSuchPhon( xmltag() + "::phon_content(" + cls + ")" );
      }
    }
    if ( !speakable() || ( hidden() && !show_hidden ) ) {
      throw NoSuchPhon( "non-speakable element: " + xmltag() );
    }

    for ( const auto& el : _data ) {
      if ( el->isinstance(PhonContent_t) && ( el->cls() == cls) ) {
	return dynamic_cast<PhonContent*>(el);
      }
      else if ( el->element_id() == Correction_t) {
	try {
	  return el->phon_content(cls,show_hidden);
	} catch ( const NoSuchPhon& e ) {
	  // continue search for other Corrections or a TextContent
	}
      }
    }
    throw NoSuchPhon( xmltag() + "::phon_content(" + cls + ")" );
  }

  //#define DEBUG_PHON

  const UnicodeString AbstractElement::phon( const string& cls,
					     TEXT_FLAGS flags ) const {
    // get the UnicodeString value of underlying elements
    // default cls="current"
    bool hidden = ( TEXT_FLAGS::HIDDEN & flags ) == TEXT_FLAGS::HIDDEN;
    bool strict = ( TEXT_FLAGS::STRICT & flags ) == TEXT_FLAGS::STRICT;
#ifdef DEBUG_PHON
    cerr << "PHON(" << cls << ") op node : " << xmltag() << " id ( " << id() << ")" << endl;
#endif
    if ( strict ) {
      return phon_content(cls)->phon();
    }
    else if ( !speakable() || ( this->hidden() && !hidden ) ) {
      throw NoSuchPhon( "NON speakable element: " + xmltag() );
    }
    else {
      UnicodeString result = deepphon( cls, flags );
      if ( result.isEmpty() ) {
	result = phon_content(cls,hidden)->phon();
      }
      if ( result.isEmpty() ) {
	throw NoSuchPhon( "on tag " + xmltag() + " nor it's children" );
      }
      return result;
    }
  }

  const UnicodeString AbstractElement::deepphon( const string& cls,
						 TEXT_FLAGS flags ) const {
    // get the UnicodeString value of underlying elements
    // default cls="current"
#ifdef DEBUG_PHON
    cerr << "deepPHON(" << cls << ") op node : " << xmltag() << " id(" << id() << ")" << endl;
#endif
#ifdef DEBUG_PHON
    cerr << "deepphon: node has " << _data.size() << " children." << endl;
#endif
    vector<UnicodeString> parts;
    vector<UnicodeString> seps;
    for ( const auto& child : _data ) {
      // try to get text dynamically from children
      // skip PhonContent elements
#ifdef DEBUG_PHON
      if ( !child->speakable() ) {
	cerr << "deepphon: node[" << child->xmltag() << "] NOT SPEAKABLE! " << endl;
      }
#endif
      if ( child->speakable() && !child->isinstance( PhonContent_t ) ) {
#ifdef DEBUG_PHON
	cerr << "deepphon:bekijk node[" << child->xmltag() << "]" << endl;
#endif
	try {
	  UnicodeString tmp = child->phon( cls );
#ifdef DEBUG_PHON
	  cerr << "deepphon found '" << tmp << "'" << endl;
#endif
	  parts.push_back(tmp);
	  // get the delimiter
	  const string& delim = child->get_delimiter();
#ifdef DEBUG_PHON
	  cerr << "deepphon:delimiter van "<< child->xmltag() << " ='" << delim << "'" << endl;
#endif
	  seps.push_back(TiCC::UnicodeFromUTF8(delim));
	} catch ( const NoSuchPhon& e ) {
#ifdef DEBUG_PHON
	  cerr << "HELAAS" << endl;
#endif
	}
      }
    }

    // now construct the result;
    UnicodeString result;
    for ( size_t i=0; i < parts.size(); ++i ) {
      result += parts[i];
      if ( i < parts.size()-1 ) {
	result += seps[i];
      }
    }
#ifdef DEBUG_TEXT
    cerr << "deepphon() for " << xmltag() << " step 3 " << endl;
#endif
    if ( result.isEmpty() ) {
      bool hidden = ( TEXT_FLAGS::HIDDEN & flags ) == TEXT_FLAGS::HIDDEN;
      try {
	result = phon_content(cls,hidden)->phon();
      }
      catch ( ... ) {
      }
    }
#ifdef DEBUG_TEXT
    cerr << "deepphontext() for " << xmltag() << " result= '" << result << "'" << endl;
#endif
    if ( result.isEmpty() ) {
      throw NoSuchPhon( xmltag() + ":(class=" + cls +"): empty!" );
    }
    return result;
  }


  vector<FoliaElement *>AbstractElement::find_replacables( FoliaElement *par ) const {
    return par->select( element_id(), sett(), SELECT_FLAGS::LOCAL );
  }

  void AbstractElement::replace( FoliaElement *child ) {
    // Appends a child element like append(), but replaces any existing child
    // element of the same type and set.
    // If no such child element exists, this will act the same as append()

    vector<FoliaElement*> replace = child->find_replacables( this );
    if ( replace.empty() ) {
      // nothing to replace, simply call append
      append( child );
    }
    else if ( replace.size() > 1 ) {
      throw runtime_error( "Unable to replace. Multiple candidates found, unable to choose." );
    }
    else {
      this->remove( replace[0], true );
      append( child );
    }
  }

  FoliaElement* AbstractElement::replace( FoliaElement *old,
					  FoliaElement* _new ) {
    // replaced old by _new
    // returns old
    // when not found does nothing and returns 0;
    // for ( auto& el: _data ) {
    //   if ( el == old ) {
    // 	el = _new;
    // 	_new->set_parent( this );
    // 	return old;
    //   }
    // }
    auto it = find_if( _data.begin(),
		       _data.end(),
		       [&]( FoliaElement *el ){ return el == old; } );
    if ( it != _data.end() ){
      *it = _new;
      _new->set_parent(this);
    }
    return 0;
  }

  void AbstractElement::insert_after( FoliaElement *pos, FoliaElement *add ){
    auto it = _data.begin();
    while ( it != _data.end() ) {
      if ( *it == pos ) {
	it = _data.insert( ++it, add );
	break;
      }
      ++it;
    }
    if ( it == _data.end() ) {
      throw runtime_error( "insert_after(): previous not found" );
    }
  }

  void FoliaElement::clear_textcontent( const string& textclass ){
    for ( size_t i=0; i < size(); ++i ){
      FoliaElement *p = index(i);
      if ( p->element_id() == TextContent_t ) {
	if ( p->cls() == textclass ){
	  this->remove(p,true);
	  break;
	}
      }
    }
  }

  TextContent *FoliaElement::settext( const string& txt,
				      const string& cls ){
    // create a TextContent child of class 'cls'
    // Default cls="current"
    if ( doc() && doc()->checktext()
	 && !isSubClass( Morpheme_t ) && !isSubClass( Phoneme_t) ){
      UnicodeString deeper_u;
      try {
	deeper_u = text( cls );
	// get deep original text: no retain tokenization, no strict
      }
      catch (...){
      }
      deeper_u = normalize_spaces( deeper_u );
      UnicodeString txt_u = TiCC::UnicodeFromUTF8( txt );
      txt_u = normalize_spaces( txt_u );
      if ( !deeper_u.isEmpty() && txt_u != deeper_u ){
	throw InconsistentText( "settext(cls=" + cls + "): deeper text differs from attempted\ndeeper='" + TiCC::UnicodeToUTF8(deeper_u) + "'\nattempted='" + txt + "'" );
      }
    }
    KWargs args;
    args["value"] = txt;
    args["class"] = cls;
    TextContent *node = new TextContent( args, doc() );
    replace( node );
    return node;
  }

  TextContent *FoliaElement::setutext( const UnicodeString& txt,
				       const string& cls ){
    // create a TextContent child of class 'cls'
    // Default cls="current"
    string utf8 = TiCC::UnicodeToUTF8(txt);
    return settext( utf8, cls );
  }

  TextContent *FoliaElement::settext( const string& txt,
				      int offset,
				      const string& cls ){
    // create a TextContent child of class 'cls'
    // Default cls="current"
    // sets the offset attribute.
    if ( doc() && doc()->checktext()
	 && !isSubClass( Morpheme_t ) && !isSubClass( Phoneme_t) ){
      UnicodeString deeper_u;
      try {
	deeper_u = text( cls );
	// get deep original text: no retain tokenization, no strict
      }
      catch (...){
      }
      deeper_u = normalize_spaces( deeper_u );
      UnicodeString txt_u = TiCC::UnicodeFromUTF8( txt );
      txt_u = normalize_spaces( txt_u );
      if ( !deeper_u.isEmpty() && txt_u != deeper_u ){
	throw InconsistentText( "settext(cls=" + cls + "): deeper text differs from attempted\ndeeper='" + TiCC::UnicodeToUTF8(deeper_u) + "'\nattempted='" + txt + "'" );
      }
    }
    KWargs args;
    args["value"] = txt;
    args["class"] = cls;
    args["offset"] = TiCC::toString(offset);
    TextContent *node = new TextContent( args, doc() );
    replace( node );
    return node;
  }

  TextContent *FoliaElement::setutext( const UnicodeString& txt,
				       int offset,
				       const string& cls ){
    // create a TextContent child of class 'cls'
    // Default cls="current"
    string utf8 = TiCC::UnicodeToUTF8(txt);
    return settext( utf8, offset, cls );
  }

  const string FoliaElement::description() const {
    vector<FoliaElement *> v = select( Description_t, SELECT_FLAGS::LOCAL );
    if ( v.size() == 0 ) {
      return "";
    }
    return v[0]->description();
  }

  bool AbstractElement::acceptable( ElementType t ) const {
    auto it = accepted_data().find( t );
    if ( it == accepted_data().end() ) {
      for ( const auto& et : accepted_data() ) {
	if ( folia::isSubClass( t, et ) ) {
	  return true;
	}
      }
      return false;
    }
    return true;
  }

  bool AbstractElement::addable( const FoliaElement *c ) const {
    if ( !acceptable( c->element_id() ) ) {
      string mess = "Unable to append object of type " + c->classname()
	+ " to a <" + classname() + ">";
      if ( !_id.empty() ){
	mess += " (id=" + _id + ")";
      }
      throw ValueError( mess );
    }
    if ( c->occurrences() > 0 ) {
      vector<FoliaElement*> v = select( c->element_id(), SELECT_FLAGS::LOCAL );
      size_t count = v.size();
      if ( count >= c->occurrences() ) {
	throw DuplicateAnnotationError( "Unable to add another object of type " + c->classname() + " to " + classname() + ". There are already " + TiCC::toString(count) + " instances of this type, which is the maximum." );
      }
    }
    if ( c->occurrences_per_set() > 0 &&
	 (CLASS & c->required_attributes() || c->setonly() ) ){
      vector<FoliaElement*> v = select( c->element_id(), c->sett(), SELECT_FLAGS::LOCAL );
      size_t count = v.size();
      if ( count >= c->occurrences_per_set() ) {
	throw DuplicateAnnotationError( "Unable to add another object of type " + c->classname() + " to " + classname() + ". There are already " + TiCC::toString(count) + " instances of this type and set (" + c->sett() + "), which is the maximum." );
      }
    }
    if ( c->parent() &&
	 !( c->element_id() == WordReference_t
	    || c->referable() ) ){
      throw XmlError( "attempt to reconnect node " + c->classname() + "("
		      + c->id()
		      + ") to a " + classname() + " node, id=" + _id
		      + ", it was already connected to a "
		      +  c->parent()->classname() + " id=" + c->parent()->id() );
    }
#ifdef NOT_WORKING
    // this fails. needs attention
    if ( c->element_id() == WordReference_t ){
      string tval = atts["t"];
      if ( !tval.empty() ){
      	string tc = ref->textclass();
      	string rtval = ref->str(tc);
      	if ( tval != rtval ){
      	  throw XmlError( "WordReference id=" + id + " has another value for "
      			  + "the t attribute than it's reference. ("
      			  + tval + " versus " + rtval + ")" );
      	}
      }
    }
#endif
    if ( c->element_id() == TextContent_t
	 && element_id() == Word_t ) {
      string val = c->str();
      val = trim( val );
      if ( val.empty() ) {
     	throw ValueError( "attempt to add an empty <t> to word: " + _id );
      }
    }
    if ( c->element_id() == TextContent_t ){
      string cls = c->cls();
      string st = c->sett();
      vector<TextContent*> tmp = select<TextContent>( st, false );
      if ( !tmp.empty() ) {
	for( const auto& t : tmp ){
	  if ( t->cls() == cls ){
	    throw DuplicateAnnotationError( "attempt to add <t> with class="
					    + cls + " to element: " + _id
					    + " which already has a <t> with that class" );
	  }
	}
      }
      check_append_text_consistency( c );
    }
    return true;
  }

  void AbstractElement::assignDoc( Document* the_doc ) {
    // attach a document-less FoliaElement (tree) to its doc
    // needs checking for correct annotation_type
    // also register the ID
    // then recurse into the children
    if ( !_mydoc ) {
      _mydoc = the_doc;
      if ( annotation_type() != AnnotationType::NO_ANN
	   && !the_doc->version_below( 2, 0 )
	   && the_doc->is_undeclared( annotation_type() ) ){
	// cerr << "assignDoc: " << this << endl;
	// cerr << "ant: " << annotation_type() << endl;
	// cerr << "set: " << _set << endl;
	// so when appending a document-less child, make sure that
	// an annotation declaration is present or added.
	if ( doc()->autodeclare() ){
	  doc()->auto_declare( annotation_type(), _set );
	}
	else {
	  throw DeclarationError( "Encountered an instance of <"
				  + xmltag()
				  + "> without a proper declaration" );
	}
      }
      string myid = id();
      if ( !_set.empty()
	   && (CLASS & required_attributes() )
	   && !_mydoc->declared( annotation_type(), _set ) ) {
	throw DeclarationError( "Set " + _set + " is used in " + xmltag()
			  + "element: " + myid + " but has no declaration " +
			  "for " + toString( annotation_type() ) + "-annotation" );
      }
      if ( !myid.empty() ) {
	_mydoc->add_doc_index( this, myid );
      }
      // assume that children also might be doc-less
      for ( const auto& el : _data ) {
	el->assignDoc( _mydoc );
      }
    }
  }

  bool AbstractElement::checkAtts() {
    if ( _id.empty()
	 && (ID & required_attributes() ) ) {
      throw ValueError( "attribute 'ID' is required for " + classname() );
    }
    if ( _set.empty()
	 && (CLASS & required_attributes() ) ) {
      throw ValueError( "attribute 'set' is required for " + classname() );
    }
    if ( _class.empty()
	 && ( CLASS & required_attributes() ) ) {
      throw ValueError( "attribute 'class' is required for " + classname() );
    }
    if ( _annotator.empty()
	 && ( ANNOTATOR & required_attributes() ) ) {
      throw ValueError( "attribute 'annotator' is required for " + classname() );
    }
    if ( _annotator_type == UNDEFINED
	 && ( ANNOTATOR & required_attributes() ) ) {
      throw ValueError( "attribute 'Annotatortype' is required for " + classname() );
    }
    if ( _confidence == -1 &&
	 ( CONFIDENCE & required_attributes() ) ) {
      throw ValueError( "attribute 'confidence' is required for " + classname() );
    }
    if ( _n.empty()
	 && ( N & required_attributes() ) ) {
      throw ValueError( "attribute 'n' is required for " + classname() );
    }
    if ( _datetime.empty()
	 && ( DATETIME & required_attributes() ) ) {
      throw ValueError( "attribute 'datetime' is required for " + classname() );
    }
    if ( _begintime.empty()
	 && ( BEGINTIME & required_attributes() ) ) {
      throw ValueError( "attribute 'begintime' is required for " + classname() );
    }
    if ( _endtime.empty()
	 && ( ENDTIME & required_attributes() ) ) {
      throw ValueError( "attribute 'endtime' is required for " + classname() );
    }
    if ( _src.empty()
	 && ( SRC & required_attributes() ) ) {
      throw ValueError( "attribute 'src' is required for " + classname() );
    }
    if ( _metadata.empty()
	 && ( METADATA & required_attributes() ) ) {
      throw ValueError( "attribute 'metadata' is required for " + classname() );
    }
    if ( _speaker.empty()
	 && ( SPEAKER & required_attributes() ) ) {
      throw ValueError( "attribute 'speaker' is required for " + classname() );
    }
    return true;
  }

  FoliaElement *AbstractElement::append( FoliaElement *child ){
    // cerr << "AbstractElement::append()" << endl;
    // cerr << "AbstractElement is" << this << endl;
    // cerr << "child = " << child << endl;
    if ( !child ){
      throw XmlError( "attempt to append an empty node to a " + classname() );
    }
    bool ok = false;
    try {
      ok = child->checkAtts();
      ok &= addable( child );
    }
    catch ( const XmlError& ) {
      // don't delete the offending child in case of illegal reconnection
      // it will be deleted by the true parent
      throw;
    }
    catch ( const exception& ) {
      delete child;
      throw;
    }
    if ( ok ) {
      if ( doc() ){
	child->assignDoc( doc() );
      }
      _data.push_back(child);
      if ( !child->parent() ) {
	child->set_parent(this);
      }
      if ( child->referable() ){
	child->increfcount();
      }
      return child->postappend();
    }
    return 0;
  }

  FoliaElement *AbstractElement::postappend( ) {
    if ( id().empty() && (ID & required_attributes()) && auto_generate_id() ){
      _id = generateId( xmltag() );
    }
    return this;
  }

  FoliaElement *TextContent::postappend( ) {
    if ( doc() ){
      if ( doc()->checktext()
	   && _offset != -1
	   && ( parent() && parent()->auth() ) ){
	doc()->cache_textcontent(this);
      }
      if ( !doc()->declared( AnnotationType::TEXT ) ){
	doc()->declare( AnnotationType::TEXT, DEFAULT_TEXT_SET );
      }
    }
    return this;
  }

  FoliaElement *PhonContent::postappend( ) {
    if ( doc() ){
      if ( doc()->checktext() && _offset != -1 ){
	doc()->cache_phoncontent(this);
      }
      if ( !doc()->declared( AnnotationType::PHON ) ){
	doc()->declare( AnnotationType::PHON, DEFAULT_PHON_SET );
      }
    }
    return this;
  }

  void AbstractElement::remove( FoliaElement *child, bool del ) {
    auto it = std::remove( _data.begin(), _data.end(), child );
    _data.erase( it, _data.end() );
    if ( del ) {
      if ( child->refcount() > 0 ){
	// dont really delete yet!
	doc()->keepForDeletion( child );
      }
      else {
	delete child;
      }
    }
    else {
      child->set_parent(0);
    }
  }

  void AbstractElement::remove( size_t pos, bool del ) {
    if ( pos < _data.size() ) {
      auto it = _data.begin();
      while ( pos > 0 ) {
	++it;
	--pos;
      }
      if ( del ) {
	if ( (*it)->refcount() > 0 ){
	  // dont really delete yet!
	  doc()->keepForDeletion( *it );
	}
	else {
	  delete *it;
	}
      }
      else {
	(*it)->set_parent(0);
      }
      _data.erase(it);
    }
  }

  FoliaElement* AbstractElement::index( size_t i ) const {
    if ( i < _data.size() ) {
      return _data[i];
    }
    throw range_error( "[] index out of range" );
  }

  FoliaElement* AbstractElement::rindex( size_t i ) const {
    if ( i < _data.size() ) {
      return _data[_data.size()-1-i];
    }
    throw range_error( "[] rindex out of range" );
  }

  vector<FoliaElement*> AbstractElement::select( ElementType et,
						 const string& st,
						 const set<ElementType>& exclude,
						 SELECT_FLAGS flag ) const {
    /// The generic 'select()' function on which all other variants are based
    ///   it searches a FoLiA node for matchins sibblings.
    /// criteria:
    /// @et: which type of element we are lookinf for
    /// @st: when no empty ("") we also must match on the 'sett' of the nodes
    /// @exclude: a set of ElementTypes to exclude from searching.
    ///            These are skipped, and NOT recursed into.
    /// @flag: determines special search stategies:
    ///        RECURSE : recurse the whole FoLia from the given node downwards
    ///                  returning all matching nodes, even within matches
    ///                  This is the default.
    ///        LOCAL   : only just look in the direct sibblings of the node
    ///        TOP_HIT : like recurse, but do NOT recurse into sibblings
    ///                  of matching node
    vector<FoliaElement*> res;
    for ( const auto& el : _data ) {
      if ( el->element_id() == et &&
	   ( st.empty() || el->sett() == st ) ) {
	res.push_back( el );
	if ( flag == SELECT_FLAGS::TOP_HIT ){
	  flag = SELECT_FLAGS::LOCAL;
	}
      }
      if ( flag != SELECT_FLAGS::LOCAL ){
	// not at this level, search deeper when recurse is true
	if ( exclude.find( el->element_id() ) == exclude.end() ) {
	  vector<FoliaElement*> tmp = el->select( et, st, exclude, flag );
	  res.insert( res.end(), tmp.begin(), tmp.end() );
	}
      }
    }
    return res;
  }

  vector<FoliaElement*> AbstractElement::select( ElementType et,
						 const string& st,
						 SELECT_FLAGS flag ) const {
    return select( et, st, default_ignore, flag );
  }

  vector<FoliaElement*> AbstractElement::select( ElementType et,
						 const set<ElementType>& exclude,
						 SELECT_FLAGS flag ) const {
    return select( et, "", exclude, flag );
  }

  vector<FoliaElement*> AbstractElement::select( ElementType et,
						 SELECT_FLAGS flag ) const {
    return select( et, "", default_ignore, flag );
  }

  void AbstractElement::unravel( set<FoliaElement*>& store ){
    resetrefcount();
    store.insert( this );
    auto dit = _data.begin();
    while ( dit != _data.end() ){
      (*dit)->unravel( store );
      dit = _data.erase(dit);
    }
  }

  FoliaElement* AbstractElement::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    setAttributes( att );
    xmlNode *p = node->children;
    while ( p ) {
      string pref;
      string ns = getNS( p, pref );
      if ( !ns.empty() && ns != NSFOLIA ){
	// skip alien nodes
	if ( doc() && doc()->debug > 2 ) {
	  cerr << "skipping non-FoLiA node: " << pref << ":" << Name(p) << endl;
	}
	p = p->next;
	continue;
      }
      if ( p->type == XML_ELEMENT_NODE ) {
	string tag = Name( p );
	FoliaElement *t = createElement( tag, doc() );
	if ( t ) {
	  if ( doc() && doc()->debug > 2 ) {
	    cerr << "created " << t << endl;
	  }
	  t = t->parseXml( p );
	  if ( t ) {
	    if ( doc() && doc()->debug > 2 ) {
	      cerr << "extend " << this << " met " << t << endl;
	    }
	    append( t );
	  }
	}
	else if ( doc() && !doc()->permissive() ){
	  throw XmlError( "FoLiA parser terminated" );
	}
      }
      else if ( p->type == XML_COMMENT_NODE ) {
	string tag = "_XmlComment";
	FoliaElement *t = createElement( tag, doc() );
	if ( t ) {
	  if ( doc() && doc()->debug > 2 ) {
	    cerr << "created " << t << endl;
	  }
	  t = t->parseXml( p );
	  if ( t ) {
	    if ( doc() && doc()->debug > 2 ) {
	      cerr << "extend " << this << " met " << t << endl;
	    }
	    append( t );
	  }
	}
      }
      else if ( p->type == XML_ENTITY_REF_NODE ){
	XmlText *t = new XmlText();
	if ( p->content ) {
	  t->setvalue( (const char*)p->content );
	}
	append( t );
	if ( doc() && doc()->debug > 2 ) {
	  cerr << "created " << t << "(" << t->text() << ")" << endl;
	  cerr << "extended " << this << " met " << t << endl;
	  cerr << "this.size()= " << size() << " t.size()=" << t->size() << endl;
	}
      }
      else if ( p->type == XML_TEXT_NODE ){
	if ( this->isSubClass( TextContent_t )
	     || this->isSubClass( PhonContent_t )
	     || this->isSubClass( AbstractTextMarkup_t ) ){
	  XmlText *t = new XmlText();
	  if ( p->content ) {
	    t->setvalue( (const char*)p->content );
	  }
	  append( t );
	  if ( doc() && doc()->debug > 2 ) {
	    cerr << "created " << t << "(" << t->text() << ")" << endl;
	    cerr << "extended " << this << " met " << t << endl;
	    cerr << "this.size()= " << size() << " t.size()=" << t->size() << endl;
	  }
	}
	else {
	  //most probably this always 'empty space'
	  string tag = "_XmlText";
	  FoliaElement *t = createElement( tag, doc() );
	  if ( t ) {
	    if ( doc() && doc()->debug > 2 ) {
	      cerr << "created " << t << endl;
	    }
	    try {
	      t = t->parseXml( p );
	    }
	    catch ( const ValueError& e ){
	      delete t;
	      t = 0;
	    }
	  }
	  if ( t ) {
	    if ( doc() && doc()->debug > 2 ) {
	      cerr << "extend " << this << " met " << t << endl;
	    }
	    append( t );
	  }
	}
      }
      p = p->next;
    }
    if ( doc() && ( doc()->checktext() || doc()->fixtext() )
	 && this->printable()
	 && !isSubClass( Morpheme_t ) && !isSubClass( Phoneme_t) ){
      vector<TextContent*> tv = select<TextContent>( false );
      // first see which text classes are present
      set<string> cls;
      for ( const auto& it : tv ){
	cls.insert( it->cls() );
      }
      // check the text for every text class
      for ( const auto& st : cls ){
	UnicodeString s1, s2;
	try {
	  s1 = text( st, TEXT_FLAGS::STRICT );  // no retain tokenization, strict
	}
	catch (...){
	}
	if ( !s1.isEmpty() ){
	  //	  cerr << "S1: " << s1 << endl;
	  try {
	    s2 = text( st, TEXT_FLAGS::NONE ); // no retain tokenization, no strict
	  }
	  catch (...){
	  }
	  //	  cerr << "S2: " << s2 << endl;
	  s1 = normalize_spaces( s1 );
	  s2 = normalize_spaces( s2 );
	  if ( !s2.isEmpty() && s1 != s2 ){
	    if ( doc()->fixtext() ){
	      //	      cerr << "FIX: " << s1 << "==>" << s2 << endl;
	      KWargs args;
	      args["value"] = TiCC::UnicodeToUTF8(s2);
	      args["class"] = st;
	      TextContent *node = new TextContent( args, doc() );
	      this->replace( node );
	    }
	    else {
	      string mess = "node " + xmltag() + "(" + id()
		+ ") has a mismatch for the text in set:" + st
		+ "\nthe element text ='" + TiCC::UnicodeToUTF8(s1)
		+ "'\n" + " the deeper text ='" + TiCC::UnicodeToUTF8(s2) + "'";
	      throw( InconsistentText( mess ) );
	    }
	  }
	}
      }
    }
    return this;
  }

  void AbstractElement::setDateTime( const string& s ) {
    Attrib supported = required_attributes() | optional_attributes();
    if ( !(DATETIME & supported) ) {
      throw ValueError("datetime is not supported for " + classname() );
    }
    else {
      string time = parseDate( s );
      if ( time.empty() ) {
	throw ValueError( "invalid datetime, must be in YYYY-MM-DDThh:mm:ss format: " + s );
      }
      _datetime = time;
    }
  }

  const string AbstractElement::getDateTime() const {
    return _datetime;
  }

  const string AbstractWord::pos( const string& st ) const {
    return annotation<PosAnnotation>( st )->cls();
  }

  const string AbstractWord::lemma( const string& st ) const {
    return annotation<LemmaAnnotation>( st )->cls();
  }

  PosAnnotation *AllowInlineAnnotation::addPosAnnotation( const KWargs& inargs ) {
    KWargs args = inargs;
    string st;
    auto it = args.find("set" );
    if ( it != args.end() ) {
      st = it->second;
    }
    string newId = args.extract("generate_id" );
    if ( newId.empty() ){
      newId = "alt-pos";
    }
    if ( has_annotation<PosAnnotation>( st ) > 0 ) {
      // ok, there is already one, so create an Alternative
      KWargs kw;
      kw["xml:id"] = generateId( newId );
      if ( !doc()->declared( AnnotationType::ALTERNATIVE ) ){
	doc()->declare( AnnotationType::ALTERNATIVE, "" );
      }
      Alternative *alt = new Alternative( kw, doc() );
      append( alt );
      return alt->addAnnotation<PosAnnotation>( args );
    }
    else {
      return addAnnotation<PosAnnotation>( args );
    }
  }

  PosAnnotation* AllowInlineAnnotation::getPosAnnotations( const string& st,
							   vector<PosAnnotation*>& alts ) const {
    PosAnnotation *res = annotation<PosAnnotation>( st ); // may be 0
    alts.clear();
    // now search for alternatives
    vector<Alternative *> alt_nodes = select<Alternative>( AnnoExcludeSet );
    for ( const auto& alt : alt_nodes ){
      if ( alt->size() > 0 ) { // child elements?
	for ( size_t j=0; j < alt->size(); ++j ) {
	  if ( alt->index(j)->element_id() == PosAnnotation_t &&
	       ( st.empty() || alt->index(j)->sett() == st ) ) {
	    alts.push_back( dynamic_cast<PosAnnotation*>(alt->index(j)) );
	  }
	}
      }
    }
    return res;
  }

  LemmaAnnotation *AllowInlineAnnotation::addLemmaAnnotation( const KWargs& inargs ) {
    KWargs args = inargs;
    string st;
    auto it = args.find("set" );
    if ( it != args.end() ) {
      st = it->second;
    }
    string newId = args.extract("generate_id" );
    if ( newId.empty() ){
      newId = "alt-lem";
    }
    if ( has_annotation<LemmaAnnotation>( st ) > 0 ) {
      // ok, there is already one, so create an Alternative
      KWargs kw;
      kw["xml:id"] = generateId( newId );
      if ( !doc()->declared( AnnotationType::ALTERNATIVE ) ){
	doc()->declare( AnnotationType::ALTERNATIVE, "" );
      }
      Alternative *alt = new Alternative( kw, doc() );
      append( alt );
      return alt->addAnnotation<LemmaAnnotation>( args );
    }
    else {
      return addAnnotation<LemmaAnnotation>( args );
    }
  }

  LemmaAnnotation* AllowInlineAnnotation::getLemmaAnnotations( const string& st,
							       vector<LemmaAnnotation*>& alts ) const {
    alts.clear();
    LemmaAnnotation *res = annotation<LemmaAnnotation>( st ); // may be 0 !
    // also search alternatives
    vector<Alternative *> alt_nodes = select<Alternative>( AnnoExcludeSet );
    for ( const auto& alt : alt_nodes ){
      if ( alt->size() > 0 ) { // child elements?
	for ( size_t j =0; j < alt->size(); ++j ) {
	  if ( alt->index(j)->element_id() == LemmaAnnotation_t &&
	       ( st.empty() || alt->index(j)->sett() == st ) ) {
	    alts.push_back( dynamic_cast<LemmaAnnotation*>(alt->index(j)) );
	  }
	}
      }
    }
    return res;
  }

  MorphologyLayer *Word::addMorphologyLayer( const KWargs& inargs ) {
    KWargs args = inargs;
    string st;
    auto it = args.find("set" );
    if ( it != args.end() ) {
      st = it->second;
    }
    string newId = args.extract("generate_id" );
    if ( newId.empty() ){
      newId = "alt-mor";
    }
    if ( has_annotation<MorphologyLayer>( st ) > 0 ) {
      // ok, there is already one, so create an Alternative
      KWargs kw;
      kw["xml:id"] = generateId( newId );
      if ( !doc()->declared( AnnotationType::ALTERNATIVE ) ){
	doc()->declare( AnnotationType::ALTERNATIVE, "" );
      }
      Alternative *alt = new Alternative( kw, doc() );
      append( alt );
      return alt->addAnnotation<MorphologyLayer>( args );
    }
    else {
      return addAnnotation<MorphologyLayer>( args );
    }
  }

  MorphologyLayer *Word::getMorphologyLayers( const string& st,
					      vector<MorphologyLayer*>& alts ) const {
    alts.clear();
    MorphologyLayer *res = annotation<MorphologyLayer>( st ); // may be 0
    // now search for alternatives
    vector<Alternative *> alt_nodes = select<Alternative>( AnnoExcludeSet );
    for ( const auto& alt : alt_nodes ){
      if ( alt->size() > 0 ) { // child elements?
	for ( size_t j =0; j < alt->size(); ++j ) {
	  if ( alt->index(j)->element_id() == MorphologyLayer_t &&
	       ( st.empty() || alt->index(j)->sett() == st ) ) {
	    alts.push_back( dynamic_cast<MorphologyLayer*>(alt->index(j)) );
	  }
	}
      }
    }
    return res;
  }

  Sentence *AbstractElement::addSentence( const KWargs& args ) {
    Sentence *res = 0;
    KWargs kw = args;
    if ( !kw.is_present("xml:id") ){
      string id = generateId( "s" );
      kw["xml:id"] = id;
    }
    try {
      res = new Sentence( kw, doc() );
    }
    catch( const DuplicateIDError& e ) {
      delete res;
      throw;
    }
    append( res );
    return res;
  }

  Word *AbstractElement::addWord( const KWargs& args ) {
    Word *res = new Word( doc() );
    KWargs kw = args;
    if ( !kw.is_present("xml:id") ){
      string id = generateId( "w" );
      kw["xml:id"] = id;
    }
    try {
      res->setAttributes( kw );
    }
    catch( const DuplicateIDError& e ) {
      delete res;
      throw;
    }
    append( res );
    return res;
  }

  const string& Quote::get_delimiter( bool retaintok ) const {
#ifdef DEBUG_TEXT_DEL
    cerr << "IN " << xmltag() << "::get_delimiter (" << retaintok << ")" << endl;
#endif
    const vector<FoliaElement*>& data = this->data();
    auto it = data.rbegin();
    while ( it != data.rend() ) {
      if ( (*it)->isinstance( Sentence_t ) ) {
	// if a quote ends in a sentence, we don't want any delimiter
#ifdef DEBUG_TEXT_DEL
	cerr << "OUT " << xmltag() << "::get_delimiter ==>''" << endl;
#endif
	return EMPTY_STRING;
      }
      else {
	const string& res = (*it)->get_delimiter( retaintok );
#ifdef DEBUG_TEXT_DEL
	cerr << "OUT " << xmltag() << "::get_delimiter ==> '"
	     << res << "'" << endl;
#endif
	return res;
      }
      ++it;
    }
    static const string SPACE = " ";
    return SPACE;
  }

  vector<Word*> Quote::wordParts() const {
    vector<Word*> result;
    for ( const auto& pnt : data() ) {
      if ( pnt->isinstance( Word_t ) ) {
	result.push_back( dynamic_cast<Word*>(pnt) );
      }
      else if ( pnt->isinstance( Sentence_t ) ) {
	KWargs args;
	args["text"] = pnt->id();
	PlaceHolder *p = new PlaceHolder( args, doc() );
	doc()->keepForDeletion( p );
	result.push_back( p );
      }
      else if ( pnt->isinstance( Quote_t ) ) {
	vector<Word*> tmp = pnt->wordParts();
	result.insert( result.end(), tmp.begin(), tmp.end() );
      }
      else if ( pnt->isinstance( Description_t ) ) {
	// ignore
      }
      else {
	throw XmlError( "Word or Sentence expected in Quote. got: "
			+ pnt->classname() );
      }
    }
    return result;
  }

  vector<Word*> Sentence::wordParts() const {
    vector<Word*> result;
    for ( const auto& pnt : data() ) {
      if ( pnt->isinstance( Word_t ) ) {
	result.push_back( dynamic_cast<Word*>(pnt) );
      }
      else if ( pnt->isinstance( Quote_t ) ) {
	vector<Word*> v = pnt->wordParts();
	result.insert( result.end(), v.begin(),v.end() );
      }
      else {
	// skip all other stuff. Is there any?
      }
    }
    return result;
  }

  Correction *Sentence::splitWord( FoliaElement *orig, FoliaElement *p1, FoliaElement *p2, const KWargs& args ) {
    vector<FoliaElement*> ov;
    ov.push_back( orig );
    vector<FoliaElement*> nv;
    nv.push_back( p1 );
    nv.push_back( p2 );
    return correctWords( ov, nv, args );
  }

  Correction *Sentence::mergewords( FoliaElement *nw,
				    const vector<FoliaElement *>& orig,
				    const string& args ) {
    vector<FoliaElement*> nv;
    nv.push_back( nw );
    return correctWords( orig, nv, getArgs(args) );
  }

  Correction *Sentence::deleteword( FoliaElement *w,
				    const string& args ) {
    vector<FoliaElement*> ov;
    ov.push_back( w );
    vector<FoliaElement*> nil1;
    return correctWords( ov, nil1, getArgs(args) );
  }

  Correction *Sentence::insertword( FoliaElement *w,
				    FoliaElement *p,
				    const string& args ) {
    if ( !p || !p->isinstance( Word_t ) ) {
      throw runtime_error( "insertword(): previous is not a Word " );
    }
    if ( !w || !w->isinstance( Word_t ) ) {
      throw runtime_error( "insertword(): new word is not a Word " );
    }
    KWargs kwargs;
    kwargs["text"] = "dummy";
    kwargs["xml:id"] = "dummy";
    Word *dummy = new Word( kwargs );
    dummy->set_parent( this ); // we create a dummy Word.
    // now we insert it as member of the Sentence.
    // This makes correctWords() happy
    AbstractElement::insert_after( p, dummy );
    vector<FoliaElement *> ov;
    ov.push_back( dummy );
    vector<FoliaElement *> nv;
    nv.push_back( w );
    // so we attempt to 'correct' the dummy word into w
    return correctWords( ov, nv, getArgs(args) );
  }

  Correction *Sentence::correctWords( const vector<FoliaElement *>& orig,
				      const vector<FoliaElement *>& _new,
				      const KWargs& argsin ) {
    // Generic correction method for words. You most likely want to use the helper functions
    //      splitword() , mergewords(), deleteword(), insertword() instead

    // sanity check:
    for ( const auto& org : orig ) {
      if ( !org || !org->isinstance( Word_t) ) {
	throw runtime_error("Original word is not a Word instance" );
      }
      else if ( org->sentence() != this ) {
	throw runtime_error( "Original not found as member of sentence!");
      }
    }
    for ( const auto& nw : _new ) {
      if ( ! nw->isinstance( Word_t) ) {
	throw runtime_error("new word is not a Word instance" );
      }
    }
    auto ait = argsin.find("suggest");
    if ( ait != argsin.end() && ait->second == "true" ) {
      FoliaElement *sugg = new Suggestion();
      for ( const auto& nw : _new ) {
	sugg->append( nw );
      }
      vector<FoliaElement *> nil1;
      vector<FoliaElement *> nil2;
      vector<FoliaElement *> sv;
      vector<FoliaElement *> tmp = orig;
      sv.push_back( sugg );
      KWargs args = argsin;
      args.erase("suggest");
      return correct( nil1, tmp, nil2, sv, args );
    }
    else {
      vector<FoliaElement *> nil1;
      vector<FoliaElement *> nil2;
      vector<FoliaElement *> o_tmp = orig;
      vector<FoliaElement *> n_tmp = _new;
      return correct( o_tmp, nil1, n_tmp, nil2, argsin );
    }
  }

  void TextContent::setAttributes( const KWargs& args ) {
    KWargs kwargs = args; // need to copy
    auto it = kwargs.find( "value" );
    if ( it != kwargs.end() ) {
      string value = it->second;
      kwargs.erase(it);
      if ( value.empty() ) {
	// can this ever happen?
	throw ValueError( "TextContent: 'value' attribute may not be empty." );
      }
      XmlText *t = new XmlText();
      t->setvalue( value );
      append( t );
    }
    it = kwargs.find( "offset" );
    if ( it != kwargs.end() ) {
      _offset = stringTo<int>(it->second);
      kwargs.erase(it);
    }
    else
      _offset = -1;
    it = kwargs.find( "ref" );
    if ( it != kwargs.end() ) {
      _ref = it->second;
      kwargs.erase(it);
    }
    it = kwargs.find( "class" );
    if ( it == kwargs.end() ) {
      kwargs["class"] = "current";
    }
    AbstractElement::setAttributes(kwargs);
  }

  void PhonContent::setAttributes( const KWargs& args ) {
    KWargs kwargs = args; // need to copy
    auto it = kwargs.find( "offset" );
    if ( it != kwargs.end() ) {
      _offset = stringTo<int>(it->second);
      kwargs.erase(it);
    }
    else
      _offset = -1;
    if ( kwargs.is_present( "ref" ) ) {
      throw NotImplementedError( "ref attribute in PhonContent" );
    }
    if ( !kwargs.is_present( "class" ) ){
      kwargs["class"] = "current";
    }
    AbstractElement::setAttributes(kwargs);
  }

  FoliaElement *TextContent::find_default_reference() const {
    int depth = 0;
    FoliaElement *p = parent();
    while ( p ){
      if ( p->isSubClass( String_t )
	   || p->isSubClass( AbstractStructureElement_t )
	   || p->isSubClass( AbstractSubtokenAnnotation_t ) ){
	if ( ++depth == 2 ){
	  return p;
	}
      }
      p = p->parent();
    }
    return 0;
  }

  FoliaElement *TextContent::get_reference() const {
    FoliaElement *ref = 0;
    if ( _offset == -1 ){
      return 0;
    }
    else if ( !_ref.empty() ){
      try{
	ref = (*doc())[_ref];
      }
      catch (...){
      }
    }
    else {
      ref = find_default_reference();
    }
    if ( !ref ){
      throw UnresolvableTextContent( "Default reference for textcontent not found!" );
    }
    else if ( !ref->hastext( cls() ) ){
      throw UnresolvableTextContent( "Reference (ID " + _ref + ") has no such text (class=" + cls() + ")" );
    }
    else if ( doc()->checktext() || doc()->fixtext() ){
      UnicodeString mt = this->text( this->cls(), TEXT_FLAGS::STRICT );
      UnicodeString pt = ref->text( this->cls(), TEXT_FLAGS::STRICT );
      UnicodeString sub( pt, this->offset(), mt.length() );
      if ( mt != sub ){
	if ( doc()->fixtext() ){
	  int pos = pt.indexOf( mt );
	  if ( pos < 0 ){
	    throw UnresolvableTextContent( "Reference (ID " + ref->id() +
					   ",class=" + cls()
					   + " found, but no substring match "
					   + TiCC::UnicodeToUTF8(mt)
					   + " in " +  TiCC::UnicodeToUTF8(pt) );
	  }
	  else {
	    this->set_offset( pos );
	  }
	}
	else {
	  throw UnresolvableTextContent( "Reference (ID " + ref->id() +
					 ",class='" + cls()
					 + "') found, but no text match at "
					 + "offset=" + TiCC::toString(offset())
					 + " Expected '" + TiCC::UnicodeToUTF8(mt)
					 + "' but got '" +  TiCC::UnicodeToUTF8(sub) + "'" );
	}
      }
    }
    return ref;
  }

  KWargs TextContent::collectAttributes() const {
    KWargs attribs = AbstractElement::collectAttributes();
    if ( cls() == "current" ) {
      attribs.erase( "class" );
    }
    if ( _offset >= 0 ) {
      attribs["offset"] = TiCC::toString( _offset );
    }
    if ( !_ref.empty() ) {
      attribs["ref"] = _ref;
    }
    return attribs;
  }

  FoliaElement *PhonContent::find_default_reference() const {
    int depth = 0;
    FoliaElement *p = parent();
    while ( p ){
      if ( p->isSubClass( AbstractStructureElement_t )
	   || p->isSubClass( AbstractInlineAnnotation_t ) ){
	if ( ++depth == 2 ){
	  return p;
	}
      }
      p = p->parent();
    }
    return 0;
  }

  FoliaElement *PhonContent::get_reference() const {
    FoliaElement *ref = 0;
    if ( _offset == -1 ){
      return 0;
    }
    else if ( !_ref.empty() ){
      try{
	ref = (*doc())[_ref];
      }
      catch (...){
      }
    }
    else {
      ref = find_default_reference();
    }
    if ( !ref ){
      throw UnresolvableTextContent( "Default reference for phonetic content not found!" );
    }
    else if ( !ref->hasphon( cls() ) ){
      throw UnresolvableTextContent( "Reference (ID " + _ref + ") has no such phonetic content (class=" + cls() + ")" );
    }
    else if ( doc()->checktext() || doc()->fixtext() ){
      UnicodeString mt = this->phon( this->cls() );
      UnicodeString pt = ref->phon( this->cls() );
      UnicodeString sub( pt, this->offset(), mt.length() );
      if ( mt != sub ){
	if ( doc()->fixtext() ){
	  int pos = pt.indexOf( mt );
	  if ( pos < 0 ){
	    throw UnresolvableTextContent( "Reference (ID " + ref->id() +
					   ",class=" + cls()
					   + " found, but no substring match "
					   + TiCC::UnicodeToUTF8(mt)
					   + " in " +  TiCC::UnicodeToUTF8(pt) );
	  }
	  else {
	    this->set_offset( pos );
	  }
	}
	else {
	  throw UnresolvableTextContent( "Reference (ID " + ref->id() +
					 ",class=" + cls()
					 + " found, but no text match at "
					 + "offset=" + TiCC::toString(offset())
					 + " Expected " + TiCC::UnicodeToUTF8(mt)
					 + " but got " +  TiCC::UnicodeToUTF8(sub) );
	}
      }
    }
    return ref;
  }

  KWargs PhonContent::collectAttributes() const {
    KWargs attribs = AbstractElement::collectAttributes();
    if ( cls() == "current" ) {
      attribs.erase( "class" );
    }
    if ( _offset >= 0 ) {
      attribs["offset"] = TiCC::toString( _offset );
    }
    return attribs;
  }

  void Linebreak::setAttributes( const KWargs& args_in ){
    KWargs args = args_in;
    auto it = args.find( "pagenr" );
    if ( it != args.end() ) {
      _pagenr = it->second;
      args.erase( it );
    }
    it = args.find( "linenr" );
    if ( it != args.end() ) {
      _linenr = it->second;
      args.erase( it );
    }
    it = args.find( "newpage" );
    if ( it != args.end() ) {
      _newpage = ( it->second == "yes" );
      args.erase( it );
    }
    AbstractElement::setAttributes( args );
  }

  KWargs Linebreak::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( ! _linenr.empty() ){
      atts["linenr"] = _linenr;
    }
    if ( ! _pagenr.empty() ){
      atts["pagenr"] = _pagenr;
    }
    if ( _newpage ){
      atts["newpage"] = "yes";
    }
    return atts;
  }

  vector<FoliaElement *>TextContent::find_replacables( FoliaElement *par ) const {
    vector<FoliaElement *> result;
    vector<TextContent*> v = par->FoliaElement::select<TextContent>( sett(),
								     false );
    copy_if( v.begin(),
	     v.end(),
	     back_inserter(result),
	     [&]( FoliaElement *el ){ return el->cls() == cls(); } );
    return result;
  }

  const UnicodeString PhonContent::phon( const string& cls,
					 TEXT_FLAGS ) const {
    // get the UnicodeString value of underlying elements
    // default cls="current"
#ifdef DEBUG_PHON
    cerr << "PhonContent::PHON(" << cls << ") " << endl;
#endif
    UnicodeString result;
    for ( const auto& el : data() ) {
      // try to get text dynamically from children
#ifdef DEBUG_PHON
      cerr << "PhonContent: bekijk node[" << el->str(cls) << endl;
#endif
      try {
#ifdef DEBUG_PHON
	cerr << "roep text(" << cls << ") aan op " << el << endl;
#endif
	UnicodeString tmp = el->text( cls );
#ifdef DEBUG_PHON
	cerr << "PhonContent found '" << tmp << "'" << endl;
#endif
	result += tmp;
      } catch ( const NoSuchPhon& e ) {
#ifdef DEBUG_TEXT
	cerr << "PhonContent::HELAAS" << endl;
#endif
      }
    }
    result.trim();
#ifdef DEBUG_PHON
    cerr << "PhonContent return " << result << endl;
#endif
    return result;
  }

  const string AllowGenerateID::generateId( const string& tag ){
    // generate an new ID using my ID
    // if no ID, look upward.
    string nodeId = id();
    // cerr << "node: " << this << endl;
    // cerr << "ID=" << nodeId << endl;
    if ( nodeId.empty() ){
      FoliaElement *par = parent();
      if ( !par ){
	throw XmlError( "unable to generate an ID. No StructureElement parent found?" );
      }
      // cerr << "call on parent:" << par << endl;
      return par->generateId( tag );
    }
    else {
      int max = 0;
      if ( !tag.empty() ) {
	max = ++id_map[tag];
      }
      // cerr << "MAX = " << max << endl;
      string id = nodeId + '.' + tag + '.' +  TiCC::toString( max );
      // cerr << "new id = " << id << endl;
      return id;
    }
  }

  void AllowGenerateID::setMaxId( FoliaElement *child ) {
    if ( !child->id().empty() && !child->xmltag().empty() ) {
      vector<string> parts = TiCC::split_at( child->id(), "." );
      if ( !parts.empty() ) {
	string val = parts.back();
	int i;
	try {
	  i = stringTo<int>( val );
	}
	catch ( const exception& ) {
	  // no number, so assume some user defined id
	  return;
	}
	const auto& it = id_map.find( child->xmltag() );
	if ( it == id_map.end() ) {
	  id_map[child->xmltag()] = i;
	}
	else {
	  if ( it->second < i ) {
	    it->second = i;
	  }
	}
      }
    }
  }

  //#define DEBUG_CORRECT 1

  Correction * AllowCorrections::correct( const vector<FoliaElement*>& _original,
					  const vector<FoliaElement*>& current,
					  const vector<FoliaElement*>& _newv,
					  const vector<FoliaElement*>& _suggestions,
					  const KWargs& args_in ) {
#ifdef DEBUG_CORRECT
    cerr << "correct " << this << endl;
    cerr << "original= " << _original << endl;
    cerr << "current = " << current << endl;
    cerr << "new     = " << _newv << endl;
    cerr << "suggestions     = " << _suggestions << endl;
    cerr << "args in     = " << args_in << endl;
#endif
    // Apply a correction
    Document *doc = this->doc();
    Correction *corr = 0;
    bool hooked = false;
    New *addnew = 0;
    KWargs args = args_in;
    vector<FoliaElement*> original = _original;
    vector<FoliaElement*> _new = _newv;
    vector<FoliaElement*> suggestions = _suggestions;
    auto it = args.find("new");
    if ( it != args.end() ) {
      KWargs my_args;
      my_args["value"] = it->second;
      TextContent *t = new TextContent( my_args, doc );
      _new.push_back( t );
      args.erase( it );
    }
    it = args.find("suggestion");
    if ( it != args.end() ) {
      KWargs my_args;
      my_args["value"] = it->second;
      TextContent *t = new TextContent( my_args, doc );
      suggestions.push_back( t );
      args.erase( it );
    }
    it = args.find("reuse");
    if ( it != args.end() ) {
      // reuse an existing correction instead of making a new one
      try {
	corr = dynamic_cast<Correction*>(doc->index(it->second));
      }
      catch ( const exception& e ) {
	throw ValueError("reuse= must point to an existing correction id!");
      }
      if ( !corr->isinstance( Correction_t ) ) {
	throw ValueError("reuse= must point to an existing correction id!");
      }
      hooked = true;
      if ( !_new.empty() && corr->hasCurrent() ) {
	// can't add new if there's current, so first set original to current, and then delete current

	if ( !current.empty() ) {
	  throw runtime_error( "Can't set both new= and current= !");
	}
	if ( original.empty() ) {
	  FoliaElement *cur = corr->getCurrent();
	  original.push_back( cur );
	  corr->remove( cur, false );
	}
      }
    }
    else {
      KWargs args2 = args;
      args2.erase("suggestion" );
      args2.erase("suggestions" );
      string id = generateId( "correction" );
      args2["xml:id"] = id;
      corr = new Correction( args2, doc );
    }
#ifdef DEBUG_CORRECT
    cerr << "now corr= " << corr << endl;
#endif
    if ( !current.empty() ) {
      if ( !original.empty() || !_new.empty() ) {
	throw runtime_error("When setting current=, original= and new= can not be set!");
      }
      for ( const auto& cur : current ) {
	FoliaElement *add = new Current( doc );
	cur->set_parent(0);
	add->append( cur );
	corr->replace( add );
	if ( !hooked ) {
	  for ( size_t i=0; i < size(); ++i ) {
	    if ( index(i) == cur ) {
	      replace( index(i), corr );
	      hooked = true;
	    }
	  }
	}
      }
#ifdef DEBUG_CORRECT
      cerr << "now corr= " << corr << endl;
#endif
    }
    if ( !_new.empty() ) {
#ifdef DEBUG_CORRECT
      cerr << "there is new! " << endl;
#endif
      addnew = new New( doc );
      corr->append(addnew);
      for ( const auto& nw : _new ) {
	nw->set_parent(0);
	addnew->append( nw );
      }
#ifdef DEBUG_CORRECT
      cerr << "after adding " << corr << endl;
#endif
      vector<Current*> v = corr->FoliaElement::select<Current>();
      //delete current if present
      for ( const auto& cur:v ) {
	corr->remove( cur, false );
      }
#ifdef DEBUG_CORRECT
      cerr << "after removing cur " << corr << endl;
#endif
    }
    if ( !original.empty() ) {
#ifdef DEBUG_CORRECT
      cerr << "there is original! " << endl;
#endif
      FoliaElement *add = new Original( doc );
      corr->replace(add);
#ifdef DEBUG_CORRECT
      cerr << " corr after replace " << corr << endl;
      cerr << " new original= " << add << endl;
#endif
      for ( const auto& org: original ) {
#ifdef DEBUG_CORRECT
	cerr << " examine org " << org << endl;
#endif
	bool dummyNode = ( org->id() == "dummy" );
	if ( !dummyNode ) {
	  org->set_parent(0);
	  add->append( org );
	}
#ifdef DEBUG_CORRECT
	cerr << " NOW original= " << add << endl;
#endif
	for ( size_t i=0; i < size(); ++i ) {
#ifdef DEBUG_CORRECT
	  cerr << "in loop, bekijk " << index(i) << endl;
#endif
	  if ( index(i) == org ) {
#ifdef DEBUG_CORRECT
	    cerr << "OK hit on ORG" << endl;
#endif
	    if ( !hooked ) {
#ifdef DEBUG_CORRECT
	      cerr << "it isn't hooked!" << endl;
	      FoliaElement * tmp = replace( index(i), corr );
	      cerr << " corr after replace " << corr << endl;
	      cerr << " replaced " << tmp << endl;
#else
	      replace( index(i), corr );
#endif
	      hooked = true;
	    }
	    else {
#ifdef DEBUG_CORRECT
	      cerr << " corr before remove " << corr << endl;
	      cerr << " remove  " << org << endl;
#endif
	      this->remove( org, false );
#ifdef DEBUG_CORRECT
	      cerr << " corr after remove " << corr << endl;
#endif
	    }
	  }
	}
      }
    }
    else if ( addnew ) {
      // original not specified, find automagically:
      vector<FoliaElement *> orig;
#ifdef DEBUG_CORRECT
      cerr << "start to look for original " << endl;
#endif
      for ( size_t i=0; i < len(addnew); ++ i ) {
	FoliaElement *p = addnew->index(i);
#ifdef DEBUG_CORRECT
	cerr << "bekijk " << p << endl;
#endif
	vector<FoliaElement*> v = p->find_replacables( this );
	// for ( const auto& el: v ) {
	//   orig.push_back( el );
	// }
	copy( v.begin(), v.end(), back_inserter(orig) );
      }
      if ( orig.empty() ) {
	throw runtime_error( "No original= specified and unable to automatically infer");
      }
      else {
#ifdef DEBUG_CORRECT
	cerr << "we seem to have some originals! " << endl;
#endif
	FoliaElement *add = new Original( doc );
#ifdef DEBUG_CORRECT
	cerr << "corr before adding new original! " << corr << endl;
#endif
	corr->replace(add);
#ifdef DEBUG_CORRECT
	cerr << "corr after adding new original! " << corr << endl;
	cerr << "now parent = " << add->parent() << endl;
#endif

	for ( const auto& org: orig ) {
#ifdef DEBUG_CORRECT
	  cerr << " examine original : " << org << endl;
	  cerr << "with parent = " << org->parent() << endl;
#endif
	  // first we lookup org in our data and remove it there
	  for ( size_t i=0; i < size(); ++i ) {
#ifdef DEBUG_CORRECT
	    cerr << "in loop, bekijk " << index(i) << endl;
#endif
	    if ( index(i) == org ) {
#ifdef DEBUG_CORRECT
	      cerr << "found original " << endl;
#endif
	      if ( !hooked ) {
#ifdef DEBUG_CORRECT
		cerr << "it isn't hooked!" << endl;
		FoliaElement *tmp = replace( index(i), corr );
		cerr << " corr after replace " << corr << endl;
		cerr << " replaced " << tmp << endl;
#else
		replace( index(i), corr );
#endif

		hooked = true;
	      }
	      else {
#ifdef DEBUG_CORRECT
		cerr << " corr before remove " << corr << endl;
		cerr << " remove  " << org << endl;
#endif
		this->remove( org, false );
#ifdef DEBUG_CORRECT
		cerr << " corr after remove " << corr << endl;
#endif
	      }
	    }
	  }
	  // now we conect org to the new original node
	  org->set_parent( 0 );
	  add->append( org );
#ifdef DEBUG_CORRECT
	  cerr << " add after append : " << add << endl;
	  cerr << "parent = " << org->parent() << endl;
#endif
	}
	vector<Current*> v = corr->FoliaElement::select<Current>();
	//delete current if present
	for ( const auto& cur: v ) {
#ifdef DEBUG_CORRECT
	  cerr << " remove cur=" << cur << endl;
#endif
	  this->remove( cur, false );
	}
      }
    }
#ifdef DEBUG_CORRECT
    cerr << " corr after edits " << corr << endl;
#endif
    if ( addnew ) {
      for ( const auto& org : original ) {
#ifdef DEBUG_CORRECT
	cerr << " remove  " << org << endl;
#endif
	bool dummyNode = ( org->id() == "dummy" );
	corr->remove( org, dummyNode );
      }
    }
#ifdef DEBUG_CORRECT
    cerr << " corr after removes " << corr << endl;
#endif
    if ( !suggestions.empty() ) {
      if ( !hooked ) {
	append(corr);
      }
      for ( const auto& sug : suggestions ) {
	if ( sug->isinstance( Suggestion_t ) ) {
	  sug->set_parent(0);
	  corr->append( sug );
	}
	else {
	  FoliaElement *add = new Suggestion( doc );
	  sug->set_parent(0);
	  add->append( sug );
	  corr->append( add );
	}
      }
    }

    it = args.find("reuse");
    if ( it != args.end() ) {
      it = args.find("annotator");
      if ( it != args.end() ) {
	corr->annotator( it->second );
      }
      it = args.find("annotatortype");
      if ( it != args.end() ){
	corr->annotatortype( stringTo<AnnotatorType>(it->second) );
      }
      it = args.find("confidence");
      if ( it != args.end() ) {
	corr->confidence( stringTo<double>(it->second) );
      }
    }
    return corr;
  }

  Correction *AllowCorrections::correct( const string& s ) {
    vector<FoliaElement*> nil1;
    vector<FoliaElement*> nil2;
    vector<FoliaElement*> nil3;
    vector<FoliaElement*> nil4;
    KWargs args = getArgs( s );
    //    cerr << xmltag() << "::correct() <== " << this << endl;
    Correction *tmp = correct( nil1, nil2, nil3, nil4, args );
    //    cerr << xmltag() << "::correct() ==> " << this << endl;
    return tmp;
  }

  Correction *AllowCorrections::correct( FoliaElement *_old,
					 FoliaElement *_new,
					 const vector<FoliaElement*>& sugg,
					 const KWargs& args ) {
    vector<FoliaElement *> nv;
    nv.push_back( _new );
    vector<FoliaElement *> ov;
    ov.push_back( _old );
    vector<FoliaElement *> nil;
    //    cerr << xmltag() << "::correct() <== " << this << endl;
    Correction *tmp = correct( ov, nil, nv, sugg, args );
    //    cerr << xmltag() << "::correct() ==> " << this << endl;
    return tmp;
  }

  FoliaElement *AbstractStructureElement::append( FoliaElement *child ){
    // cerr << "AbstractStructureElement::append()" << endl;
    // cerr << "AbstractStructureElement is" << this << endl;
    // cerr << "child = " << child << endl;
    AbstractElement::append( child );
    setMaxId( child );
    return child;
  }

  vector<Paragraph*> AbstractStructureElement::paragraphs() const{
    return FoliaElement::select<Paragraph>( default_ignore_structure );
  }

  vector<Sentence*> AbstractStructureElement::sentences() const{
    return FoliaElement::select<Sentence>( default_ignore_structure );
  }

  vector<Word*> AbstractStructureElement::words( const string& st ) const{
    return FoliaElement::select<Word>( st, default_ignore_structure );
  }

  Sentence *AbstractStructureElement::sentences( size_t index ) const {
    vector<Sentence*> v = sentences();
    if ( index < v.size() ) {
      return v[index];
    }
    throw range_error( "sentences(): index out of range" );
  }

  Sentence *AbstractStructureElement::rsentences( size_t index ) const {
    vector<Sentence*> v = sentences();
    if ( index < v.size() ) {
      return v[v.size()-1-index];
    }
    throw range_error( "rsentences(): index out of range" );
  }

  Paragraph *AbstractStructureElement::paragraphs( size_t index ) const {
    vector<Paragraph*> v = paragraphs();
    if ( index < v.size() ) {
      return v[index];
    }
    throw range_error( "paragraphs(): index out of range" );
  }

  Paragraph *AbstractStructureElement::rparagraphs( size_t index ) const {
    vector<Paragraph*> v = paragraphs();
    if ( index < v.size() ) {
      return v[v.size()-1-index];
    }
    throw range_error( "rparagraphs(): index out of range" );
  }

  Word *AbstractStructureElement::words( size_t index,
					 const string& st ) const {
    vector<Word*> v = words(st);
    if ( index < v.size() ) {
      return v[index];
    }
    throw range_error( "words(): index out of range" );
  }

  Word *AbstractStructureElement::rwords( size_t index,
					  const string& st ) const {
    vector<Word*> v = words(st);
    if ( index < v.size() ) {
      return v[v.size()-1-index];
    }
    throw range_error( "rwords(): index out of range" );
  }

  const Word* AbstractStructureElement::resolveword( const string& id ) const{
    const Word *result = 0;
    for ( const auto& el : data() ) {
      result = el->resolveword( id );
      if ( result ) {
	break;
      }
    }
    return result;
  }

  vector<Alternative *> AllowInlineAnnotation::alternatives( ElementType elt,
							     const string& st ) const {
    // Return a list of alternatives, either all or only of a specific type, restrained by set
    vector<Alternative *> alts = FoliaElement::select<Alternative>( AnnoExcludeSet );
    if ( elt == BASE ) {
      return alts;
    }
    else {
      vector<Alternative*> res;
      for ( const auto& alt : alts ){
	if ( alt->size() > 0 ) { // child elements?
	  for ( size_t j =0; j < alt->size(); ++j ) {
	    auto hit = alt->index(j);
	    if ( hit->element_id() == elt &&
		 ( hit->sett().empty() || hit->sett() == st ) ) {
	      res.push_back( alt ); // not the child!
	    }
	  }
	}
      }
      return res;
    }
  }

  KWargs LinkReference::collectAttributes() const {
    KWargs atts;
    atts["id"] = refId;
    atts["type"] = ref_type;
    if ( !_t.empty() ) {
      atts["t"] = _t;
    }
    return atts;
  }

  void LinkReference::setAttributes( const KWargs& argsin ) {
    KWargs args = argsin;
    auto it = args.find( "id" );
    if ( it != args.end() ) {
      refId = it->second;
      args.erase(it);
    }
    it = args.find( "type" );
    if ( it != args.end() ) {
      ref_type = it->second;
      args.erase(it);
    }
    it = args.find( "t" );
    if ( it != args.end() ) {
      _t = it->second;
      args.erase(it);
    }
    AbstractElement::setAttributes(args);
  }

  void Word::setAttributes( const KWargs& args_in ) {
    KWargs args = args_in;
    auto const& it = args.find( "text" );
    if ( it != args.end() ) {
      settext( it->second );
      args.erase( it );
    }
    AbstractElement::setAttributes( args );
  }

  KWargs Word::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    return atts;
  }

  const string& Word::get_delimiter( bool retaintok ) const {
    if ( space() || retaintok ) {
      return PROPS.TEXTDELIMITER;
    }
    return EMPTY_STRING;
  }

  Correction *Word::split( FoliaElement *part1, FoliaElement *part2,
			   const string& args ) {
    return sentence()->splitWord( this, part1, part2, getArgs(args) );
  }

  FoliaElement *Word::append( FoliaElement *child ){
    if ( child->isSubClass( AbstractAnnotationLayer_t ) ) {
      // sanity check, there may be no other child within the same set
      vector<FoliaElement*> v = select( child->element_id(), child->sett() );
      if ( v.empty() ) {
    	// OK!
    	return AbstractElement::append( child );
      }
      delete child;
      throw DuplicateAnnotationError( "Word::append" );
    }
    return AbstractElement::append( child );
  }

  Sentence *AbstractWord::sentence() const {
    // return the sentence this word is a part of, otherwise return null
    FoliaElement *p = parent();
    while( p ) {
      if ( p->isinstance( Sentence_t ) ) {
	return dynamic_cast<Sentence*>(p);
      }
      p = p->parent();
    }
    return 0;
  }

  Paragraph *AbstractWord::paragraph( ) const {
    // return the sentence this word is a part of, otherwise return null
    FoliaElement *p = parent();
    while( p ) {
      if ( p->isinstance( Paragraph_t ) ) {
	return dynamic_cast<Paragraph*>(p);
      }
      p = p->parent();
    }
    return 0;
  }

  Division *AbstractWord::division() const {
    // return the <div> this word is a part of, otherwise return null
    FoliaElement *p = parent();
    while( p ) {
      if ( p->isinstance( Division_t ) ) {
	return dynamic_cast<Division*>(p);
      }
      p = p->parent();
    }
    return 0;
  }

  vector<Morpheme *> AbstractWord::morphemes( const string& set ) const {
    vector<Morpheme *> result;
    vector<MorphologyLayer*> mv = FoliaElement::select<MorphologyLayer>();
    for ( const auto& mor : mv ){
      vector<Morpheme*> tmp = mor->FoliaElement::select<Morpheme>( set );
      result.insert( result.end(), tmp.begin(), tmp.end() );
    }
    return result;
  }

  Morpheme *AbstractWord::morpheme( size_t pos, const string& set ) const {
    vector<Morpheme *> tmp = morphemes( set );
    if ( pos < tmp.size() ) {
      return tmp[pos];
    }
    throw range_error( "morpheme() index out of range" );
  }

  Correction *Word::incorrection( ) const {
    // Is the Word part of a correction? If it is, it returns the Correction element, otherwise it returns 0;
    FoliaElement *p = parent();
    while ( p ) {
      if ( p->isinstance( Correction_t ) ) {
	return dynamic_cast<Correction*>(p);
      }
      else if ( p->isinstance( Sentence_t ) ){
	break;
      }
      p = p->parent();
    }
    return 0;
  }

  Word *Word::previous() const{
    Sentence *s = sentence();
    vector<Word*> words = s->words();
    for ( size_t i=0; i < words.size(); ++i ) {
      if ( words[i] == this ) {
	if ( i > 0 ) {
	  return words[i-1];
	}
	else {
	  return 0;
	}
	break;
      }
    }
    return 0;
  }

  Word *Word::next() const{
    Sentence *s = sentence();
    vector<Word*> words = s->words();
    for ( size_t i=0; i < words.size(); ++i ) {
      if ( words[i] == this ) {
	if ( i+1 < words.size() ) {
	  return words[i+1];
	}
	else {
	  return 0;
	}
	break;
      }
    }
    return 0;
  }

  vector<Word*> Word::context( size_t size,
			       const string& val ) const {
    vector<Word*> result;
    if ( size > 0 ) {
      vector<Word*> words = doc()->words();
      for ( size_t i=0; i < words.size(); ++i ) {
	if ( words[i] == this ) {
	  size_t miss = 0;
	  if ( i < size ) {
	    miss = size - i;
	  }
	  for ( size_t index=0; index < miss; ++index ) {
	    if ( val.empty() ) {
	      result.push_back( 0 );
	    }
	    else {
	      KWargs args;
	      args["text"] = val;
	      PlaceHolder *p = new PlaceHolder( args );
	      doc()->keepForDeletion( p );
	      result.push_back( p );
	    }
	  }
	  for ( size_t index=i-size+miss; index < i + size + 1; ++index ) {
	    if ( index < words.size() ) {
	      result.push_back( words[index] );
	    }
	    else {
	      if ( val.empty() ) {
		result.push_back( 0 );
	      }
	      else {
		KWargs args;
		args["text"] = val;
		PlaceHolder *p = new PlaceHolder( args );
		doc()->keepForDeletion( p );
		result.push_back( p );
	      }
	    }
	  }
	  break;
	}
      }
    }
    return result;
  }


  vector<Word*> Word::leftcontext( size_t size,
				   const string& val ) const {
    //  cerr << "leftcontext : " << size << endl;
    vector<Word*> result;
    if ( size > 0 ) {
      vector<Word*> words = doc()->words();
      for ( size_t i=0; i < words.size(); ++i ) {
	if ( words[i] == this ) {
	  size_t miss = 0;
	  if ( i < size ) {
	    miss = size - i;
	  }
	  for ( size_t index=0; index < miss; ++index ) {
	    if ( val.empty() ) {
	      result.push_back( 0 );
	    }
	    else {
	      KWargs args;
	      args["text"] = val;
	      PlaceHolder *p = new PlaceHolder( args );
	      doc()->keepForDeletion( p );
	      result.push_back( p );
	    }
	  }
	  for ( size_t index=i-size+miss; index < i; ++index ) {
	    result.push_back( words[index] );
	  }
	  break;
	}
      }
    }
    return result;
  }

  vector<Word*> Word::rightcontext( size_t size,
				    const string& val ) const {
    vector<Word*> result;
    //  cerr << "rightcontext : " << size << endl;
    if ( size > 0 ) {
      vector<Word*> words = doc()->words();
      size_t begin;
      size_t end;
      for ( size_t i=0; i < words.size(); ++i ) {
	if ( words[i] == this ) {
	  begin = i + 1;
	  end = begin + size;
	  for ( ; begin < end; ++begin ) {
	    if ( begin >= words.size() ) {
	      if ( val.empty() ) {
		result.push_back( 0 );
	      }
	      else {
		KWargs args;
		args["text"] = val;
		PlaceHolder *p = new PlaceHolder( args );
		doc()->keepForDeletion( p );
		result.push_back( p );
	      }
	    }
	    else
	      result.push_back( words[begin] );
	  }
	  break;
	}
      }
    }
    return result;
  }

  const Word* Word::resolveword( const string& id ) const {
    if ( AbstractElement::id() == id ) {
      return this;
    }
    return 0;
  }

  ElementType layertypeof( ElementType et ) {
    switch( et ) {
    case Entity_t:
    case EntitiesLayer_t:
      return EntitiesLayer_t;
    case Chunk_t:
    case ChunkingLayer_t:
      return ChunkingLayer_t;
    case SyntacticUnit_t:
    case SyntaxLayer_t:
      return SyntaxLayer_t;
    case TimeSegment_t:
    case TimingLayer_t:
      return TimingLayer_t;
    case Morpheme_t:
    case MorphologyLayer_t:
      return MorphologyLayer_t;
    case Phoneme_t:
    case PhonologyLayer_t:
      return PhonologyLayer_t;
    case CoreferenceChain_t:
    case CoreferenceLayer_t:
      return CoreferenceLayer_t;
    case Observation_t:
    case ObservationLayer_t:
      return ObservationLayer_t;
    // case Predicate_t:
    // case PredicateLayer_t:
    //   return PredicateLayer_t;
    case SentimentLayer_t:
    case Sentiment_t:
      return SentimentLayer_t;
    case StatementLayer_t:
    case Statement_t:
      return SentimentLayer_t;
    case SemanticRolesLayer_t:
    case SemanticRole_t:
      return SemanticRolesLayer_t;
    case DependenciesLayer_t:
    case Dependency_t:
      return DependenciesLayer_t;
    default:
      return BASE;
    }
  }

  vector<AbstractSpanAnnotation*> AbstractWord::findspans( ElementType et,
							   const string& st ) const {
    ElementType layertype = layertypeof( et );
    vector<AbstractSpanAnnotation *> result;
    if ( layertype != BASE ) {
      const FoliaElement *e = parent();
      if ( e ) {
	vector<FoliaElement*> v = e->select( layertype, st, SELECT_FLAGS::LOCAL );
	for ( const auto& el : v ){
	  for ( size_t k=0; k < el->size(); ++k ) {
	    FoliaElement *f = el->index(k);
	    AbstractSpanAnnotation *as = dynamic_cast<AbstractSpanAnnotation*>(f);
	    if ( as ) {
	      vector<FoliaElement*> wrefv = f->wrefs();
	      for ( const auto& wr : wrefv ){
		if ( wr == this ) {
		  result.push_back(as);
		}
	      }
	    }
	  }
	}
      }
    }
    return result;
  }

  FoliaElement* WordReference::parseXml( const xmlNode *node ) {
    KWargs atts = getAttributes( node );
    string id = atts["id"];
    if ( id.empty() ) {
      throw XmlError( "empty id in WordReference" );
    }
    if ( doc()->debug ) {
      cerr << "Found word reference: " << id << endl;
    }
    FoliaElement *ref = (*doc())[id];
    if ( ref ) {
      if ( !ref->referable() ){
	throw XmlError( "WordReference id=" + id + " refers to a non-referable word: "
			+ ref->xmltag() );
      }
      // Disabled test! should consider the textclass of the yet unknown
      // parent!
      // addable() should check this. But that is impossible!
      // we don't return a WordReference but the ref to the word!
      //
      // string tval = atts["t"];
      // if ( !tval.empty() ){
      // 	string tc = ref->textclass();
      // 	string rtval = ref->str(tc);
      // 	if ( tval != rtval ){
      // 	  throw XmlError( "WordReference id=" + id + " has another value for "
      // 			  + "the t attribute than it's reference. ("
      // 			  + tval + " versus " + rtval + ")" );
      // 	}
      // }
      ref->increfcount();
    }
    else {
      throw XmlError( "Unresolvable id " + id + " in WordReference" );
    }
    delete this;
    return ref;
  }

  FoliaElement* LinkReference::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    string val = att["id"];
    if ( val.empty() ) {
      throw XmlError( "ID required for LinkReference" );
    }
    refId = val;
    if ( doc()->debug ) {
      cerr << "Found LinkReference ID " << refId << endl;
    }
    ref_type = att["type"];
    val = att["t"];
    if ( !val.empty() ) {
      _t = val;
    }
    return this;
  }

  FoliaElement *LinkReference::resolve_element( const Relation *ref ) const {
    if ( ref->href().empty() ) {
      return (*doc())[refId];
    }
    throw NotImplementedError( "LinkReference::resolve() for external doc" );
  }

  void Relation::setAttributes( const KWargs& kwargsin ) {
    KWargs kwargs = kwargsin;
    auto it = kwargs.find( "format" );
    if ( it != kwargs.end() ) {
      _format = it->second;
      kwargs.erase( it );
    }
    AbstractElement::setAttributes(kwargs);
  }

  KWargs Relation::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( !_format.empty() && _format != "text/folia+xml" ) {
      atts["format"] = _format;
    }
    return atts;
  }

  vector<FoliaElement *> Relation::resolve() const {
    vector<FoliaElement*> result;
    vector<LinkReference*> v = FoliaElement::select<LinkReference>();
    // for ( const auto& ar : v ){
    //   result.push_back( ar->resolve_element( this ) );
    // }
    transform( v.begin(), v.end(),
	       back_inserter(result),
	       [&]( LinkReference *r ){ return r->resolve_element(this); } );
    return result;
  }

  void PlaceHolder::setAttributes( const KWargs& args ) {
    if ( !args.is_present("text") ) {
      throw ValueError("text attribute is required for " + classname() );
    }
    else if ( args.size() != 1 ) {
      throw ValueError("only the text attribute is supported for " + classname() );
    }
    Word::setAttributes( args );
  }

  const UnicodeString Figure::caption() const {
    vector<FoliaElement *> v = select(Caption_t);
    if ( v.empty() ) {
      throw NoSuchText("caption");
    }
    else {
      return v[0]->text();
    }
  }

  void Description::setAttributes( const KWargs& kwargsin ) {
    KWargs kwargs = kwargsin;
    string val = kwargs.extract( "value" );
    if ( !val.empty() ) {
      _value = val;
    }
    AbstractElement::setAttributes( kwargs );
  }

  xmlNode *Description::xml( bool, bool ) const {
    xmlNode *e = AbstractElement::xml( false, false );
    if ( !_value.empty() ){
      xmlAddChild( e, xmlNewText( (const xmlChar*)_value.c_str()) );
    }
    return e;
  }

  FoliaElement* Description::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    if ( !att.is_present("value") ) {
      att["value"] = XmlContent( node );
    }
    setAttributes( att );
    return this;
  }

  void Comment::setAttributes( const KWargs& kwargsin ) {
    KWargs kwargs = kwargsin;
    string val = kwargs.extract( "value" );
    if ( !val.empty() ) {
      _value = val;
    }
    AbstractElement::setAttributes( kwargs );
  }

  xmlNode *Comment::xml( bool, bool ) const {
    xmlNode *e = AbstractElement::xml( false, false );
    if ( !_value.empty() ){
      xmlAddChild( e, xmlNewText( (const xmlChar*)_value.c_str()) );
    }
    return e;
  }

  FoliaElement* Comment::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    if ( !att.is_present("value") ) {
      att["value"] = XmlContent( node );
    }
    setAttributes( att );
    return this;
  }

  FoliaElement *AbstractSpanAnnotation::append( FoliaElement *child ){
    if ( child->referable() ){
      // cerr << "append a word: " << child << " to " << this << endl;
      // cerr << "refcnt=" << child->refcount() << endl;
      if ( child->refcount() == 0 ){
	throw XmlError( "connecting a <w> to an <" + xmltag()
       			+ "> is forbidden, use <wref>" );
      }
    }
    AbstractElement::append( child );
    if ( child->isinstance(PlaceHolder_t) ) {
      child->increfcount();
    }
    return child;
  }

  void AbstractAnnotationLayer::assignset( FoliaElement *child ) {
    // If there is no set (yet), try to get the set from the child
    // but not if it is the default set.
    // for a Correction child, we look deeper.
    // BARF when the sets are incompatible.
    string c_set;
    if ( child->isSubClass( AbstractSpanAnnotation_t ) ) {
      string st = child->sett();
      if ( !st.empty()
	   && doc()->default_set( child->annotation_type() ) != st ) {
	c_set = st;
      }
    }
    else if ( child->isinstance(Correction_t) ) {
      Original *org = child->getOriginal();
      if ( org ) {
	for ( size_t i=0; i < org->size(); ++i ) {
	  FoliaElement *el = org->index(i);
	  if ( el->isSubClass( AbstractSpanAnnotation_t ) ) {
	    string st = el->sett();
	    if ( !st.empty()
		 && doc()->default_set( el->annotation_type() ) != st ) {
	      c_set = st;
	      break;
	    }
	  }
	}
      }
      if ( c_set.empty() ){
	New *nw = child->getNew();
	if ( nw ) {
	  for ( size_t i=0; i < nw->size(); ++i ) {
	    FoliaElement *el = nw->index(i);
	    if ( el->isSubClass( AbstractSpanAnnotation_t ) ) {
	      string st = el->sett();
	      if ( !st.empty()
		   && doc()->default_set( el->annotation_type() ) != st ) {
		c_set = st;
		break;
	      }
	    }
	  }
	}
      }
      if ( c_set.empty() ){
	auto v = child->suggestions();
	for ( const auto& el : v ) {
	  if ( el->isSubClass( AbstractSpanAnnotation_t ) ) {
	    string st = el->sett();
	    if ( !st.empty()
		 && doc()->default_set( el->annotation_type() ) != st ) {
	      c_set = st;
	      break;
	    }
	  }
	}
      }
    }
    if ( c_set.empty() ){
      return;
    }
    if ( sett().empty() ) {
      update_set( c_set );
    }
    else if ( sett() != c_set ){
      throw DuplicateAnnotationError( "appending child: " + child->xmltag()
				      + " with set='"
				      +  c_set + "' to " + xmltag()
				      + " failed while it already has set='"
				      + sett() + "'" );
    }
    doc()->incrRef( child->annotation_type(), sett() );
  }

  FoliaElement *AbstractAnnotationLayer::append( FoliaElement *child ){
    assignset( child );
    return AbstractElement::append( child );
  }

  KWargs AbstractAnnotationLayer::collectAttributes() const {
    KWargs attribs = AbstractElement::collectAttributes();
    auto it = attribs.find("set");
    if ( it != attribs.end() ) {
      attribs.erase(it);
    }
    return attribs;
  }

  xmlNode *AbstractSpanAnnotation::xml( bool recursive, bool kanon ) const {
    xmlNode *e = AbstractElement::xml( false, false );
    // append Word, Phon and Morpheme children as WREFS
    //  EXCEPT when there are NO references to it
    for ( const auto& el : data() ) {
      if ( el-referable()
	   && el->refcount() > 0 ){
	xmlNode *t = XmlNewNode( foliaNs(), "wref" );
	KWargs attribs;
	attribs["id"] = el->id();
	string txt = el->str( el->textclass() );
	if ( !txt.empty() ) {
	  attribs["t"] = txt;
	}
	addAttributes( t, attribs );
	xmlAddChild( e, t );
      }
      else {
	string at = tagToAtt( el );
	if ( at.empty() ) {
	  // otherwise handled by FoliaElement::xml() above
	  xmlAddChild( e, el->xml( recursive, kanon ) );
	}
      }
    }
    return e;
  }

  FoliaElement *Quote::append( FoliaElement *child ){
    AbstractElement::append( child );
    if ( child->isinstance(Sentence_t) ) {
      child->setAuth( false ); // Sentences under quotes are non-authoritative
    }
    return child;
  }

  xmlNode *Content::xml( bool recurse, bool ) const {
    xmlNode *e = AbstractElement::xml( recurse, false );
    xmlAddChild( e, xmlNewCDataBlock( 0,
				      (const xmlChar*)value.c_str() ,
				      value.length() ) );
    return e;
  }

  void Content::setAttributes( const KWargs& args ){
    KWargs atts = args;
    auto it = atts.find( "value" );
    if ( it != atts.end() ) {
      value = it->second;
      atts.erase( it );
    }
    AbstractElement::setAttributes( atts );
  }

  FoliaElement* Content::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    setAttributes( att );
    xmlNode *p = node->children;
    bool isCdata = false;
    bool isText = false;
    while ( p ) {
      if ( p->type == XML_CDATA_SECTION_NODE ) {
	if ( isText ) {
	  throw XmlError( "intermixing text and CDATA in Content node" );
	}
	value += (char*)p->content;
	isCdata = !value.empty();
      }
      else if ( p->type == XML_TEXT_NODE ) {
	// "empty" text nodes may appear before or after CDATA
	// we just ignore those
	string tmp = (char*)p->content;
	tmp = TiCC::trim(tmp);
	if ( !tmp.empty()
	     && isCdata ) {
	  throw XmlError( "intermixing CDATA and text in Content node" );
	}
	isText = !tmp.empty();
	value += tmp;
      }
      else if ( p->type == XML_COMMENT_NODE ) {
	string tag = "_XmlComment";
	FoliaElement *t = createElement( tag, doc() );
	if ( t ) {
	  t = t->parseXml( p );
	  append( t );
	}
      }
      p = p->next;
    }
    if ( value.empty() ) {
      throw XmlError( "CDATA or Text expected in Content node" );
    }
    return this;
  }

  const UnicodeString Correction::private_text( const string& cls,
						bool retaintok,
						bool, bool ) const {
#ifdef DEBUG_TEXT
    cerr << "TEXT(" << cls << ") op node : " << xmltag() << " id ( " << id() << ")" << endl;
#endif
    // we cannot use text_content() on New, Origibal or Current,
    // because texcontent doesn't recurse!
    for ( const auto& el : data() ) {
#ifdef DEBUG_TEXT
      cerr << "data=" << el << endl;
#endif
      if ( el->isinstance( New_t )
	   ||( el->isinstance( Original_t ) && cls != "current" )
	   || el->isinstance( Current_t ) ){
	UnicodeString result;
	try {
	  result = el->private_text( cls, retaintok, false, false );
	  return result;
	}
	catch ( ... ){
	  // try other nodes
	}
      }
    }
    throw NoSuchText( "cls=" + cls );
  }

  const string& Correction::get_delimiter( bool retaintok ) const {
    for ( const auto& el : data() ) {
      if ( el->isinstance( New_t ) || el->isinstance( Current_t ) ) {
	return el->get_delimiter( retaintok );
      }
    }
    return EMPTY_STRING;
  }

  const TextContent *Correction::text_content( const string& cls,
					       bool show_hidden ) const {
    // TODO: this implements correctionhandling::EITHER only
    for ( const auto& el : data() ) {
      if ( el->isinstance( New_t ) || el->isinstance( Current_t ) ) {
	try {
	  const TextContent *res = el->text_content( cls, show_hidden );
	  return res;
	}
	catch (...){
	}
      }
    }
    for ( const auto& el : data() ) {
      if ( el->isinstance( Original_t ) ) {
	try {
	  const TextContent *res = el->text_content( cls, show_hidden );
	  return res;
	}
	catch ( ... ){
	}
      }
      else if ( cls == "current" && el->hastext( "original" ) ){
	cerr << "text(original)= "
	     << el->text_content( cls, show_hidden )->text()<< endl;
	// hack for old and erroneous behaviour
	return el->text_content( "original", show_hidden );
      }
    }
    throw NoSuchText("wrong cls");
  }

  const PhonContent *Correction::phon_content( const string& cls,
					       bool show_hidden ) const {
    // TODO: this implements correctionhandling::EITHER only
    for ( const auto& el: data() ) {
      if ( el->isinstance( New_t ) || el->isinstance( Current_t ) ) {
	return el->phon_content( cls, show_hidden );
      }
    }
    for ( const auto& el: data() ) {
      if ( el->isinstance( Original_t ) ) {
	return el->phon_content( cls, show_hidden );
      }
    }
    throw NoSuchPhon("wrong cls");
  }

  bool Correction::hasNew( ) const {
    vector<FoliaElement*> v = select( New_t, SELECT_FLAGS::LOCAL );
    return !v.empty();
  }

  New *Correction::getNew() const {
    vector<New*> v = FoliaElement::select<New>( false );
    if ( v.empty() ) {
      return 0;
    }
    return v[0];
  }

  FoliaElement *Correction::getNew( size_t index ) const {
    New *n = getNew();
    return n->index(index);
  }

  bool Correction::hasOriginal() const {
    vector<FoliaElement*> v = select( Original_t, SELECT_FLAGS::LOCAL );
    return !v.empty();
  }

  Original *Correction::getOriginal() const {
    vector<Original*> v = FoliaElement::select<Original>( false );
    if ( v.empty() ) {
      return 0;
    }
    return v[0];
  }

  FoliaElement *Correction::getOriginal( size_t index ) const {
    Original *n = getOriginal();
    return n->index(index);
  }

  bool Correction::hasCurrent( ) const {
    vector<FoliaElement*> v = select( Current_t, SELECT_FLAGS::LOCAL );
    return !v.empty();
  }

  Current *Correction::getCurrent( ) const {
    vector<Current*> v = FoliaElement::select<Current>( false );
    if ( v.empty() ) {
      throw NoSuchAnnotation( "current" );
    }
    return v[0];
  }

  FoliaElement *Correction::getCurrent( size_t index ) const {
    Current *n = getCurrent();
    return n->index(index);
  }

  bool Correction::hasSuggestions( ) const {
    vector<Suggestion*> v = suggestions();
    return !v.empty();
  }

  vector<Suggestion*> Correction::suggestions( ) const {
    return FoliaElement::select<Suggestion>( false );
  }

  Suggestion *Correction::suggestions( size_t index ) const {
    vector<Suggestion*> v = suggestions();
    if ( v.empty() || index >= v.size() ) {
      throw NoSuchAnnotation( "suggestion" );
    }
    return v[index];
  }

  Head *Division::head() const {
    const vector<FoliaElement*>& data = this->data();
    if ( data.size() > 0 ||
	 data[0]->element_id() == Head_t ) {
      return dynamic_cast<Head*>(data[0]);
    }
    throw runtime_error( "No head" );
  }

  const string Gap::content() const {
    vector<FoliaElement*> cv = select( Content_t );
    if ( cv.empty() ) {
      throw NoSuchAnnotation( "content" );
    }
    return cv[0]->content();
  }

  Headspan *Dependency::head() const {
    vector<Headspan*> v = FoliaElement::select<Headspan>();
    if ( v.size() < 1 ) {
      throw NoSuchAnnotation( "head" );
    }
    return v[0];
  }

  DependencyDependent *Dependency::dependent() const {
    vector<DependencyDependent *> v = FoliaElement::select<DependencyDependent>();
    if ( v.empty() ) {
      throw NoSuchAnnotation( "dependent" );
    }
    return v[0];
  }

  vector<AbstractSpanAnnotation*> AbstractElement::selectSpan() const {
    vector<AbstractSpanAnnotation*> res;
    for ( const auto& el : SpanSet ) {
      vector<FoliaElement*> tmp = select( el );
      for ( auto& sp : tmp ) {
	res.push_back( dynamic_cast<AbstractSpanAnnotation*>( sp ) );
      }
    }
    return res;
  }

  vector<FoliaElement*> AbstractSpanAnnotation::wrefs() const {
    vector<FoliaElement*> res;
    for ( const auto& el : data() ) {
      ElementType et = el->element_id();
      if ( el->referable()
	   || et == WordReference_t ){
	res.push_back( el );
      }
      else {
	AbstractSpanAnnotation *as = dynamic_cast<AbstractSpanAnnotation*>(el);
	if ( as != 0 ) {
	  vector<FoliaElement*> sub = as->wrefs();
	  for ( auto& wr : sub ) {
	    res.push_back( wr );
	  }
	}
      }
    }
    return res;
  }

  FoliaElement *AbstractSpanAnnotation::wrefs( size_t pos ) const {
    vector<FoliaElement*> v = wrefs();
    if ( pos < v.size() ) {
      return v[pos];
    }
    return 0;
  }

  AbstractSpanAnnotation *AbstractAnnotationLayer::findspan( const vector<FoliaElement*>& words ) const {
    vector<AbstractSpanAnnotation*> av = selectSpan();
    for ( const auto& span : av ){
      vector<FoliaElement*> v = span->wrefs();
      if ( v.size() == words.size() ) {
	bool ok = true;
	for ( size_t n = 0; n < v.size(); ++n ) {
	  if ( v[n] != words[n] ) {
	    ok = false;
	    break;
	  }
	}
	if ( ok ) {
	  return span;
	}
      }
    }
    return 0;
  }

  bool XmlText::setvalue( const std::string& s ){
    static TiCC::UnicodeNormalizer norm;  // defaults to a NFC normalizer
    UnicodeString us = TiCC::UnicodeFromUTF8(s);
    us = norm.normalize( us );
    _value = TiCC::UnicodeToUTF8( us );
    return true;
  }

  const UnicodeString XmlText::private_text( const string&, bool, bool, bool ) const {
    return TiCC::UnicodeFromUTF8(_value);
  }

  xmlNode *XmlText::xml( bool, bool ) const {
    return xmlNewText( (const xmlChar*)_value.c_str() );
  }

  FoliaElement* XmlText::parseXml( const xmlNode *node ) {
    if ( node->content ) {
      setvalue( (const char*)node->content );
      _value = trim( _value );
    }
    if ( _value.empty() ) {
      throw ValueError( "TextContent may not be empty" );
    }
    return this;
  }

  static void error_sink(void *mydata, xmlError *error ) {
    int *cnt = (int*)mydata;
    if ( *cnt == 0 ) {
      cerr << "\nXML-error: " << error->message << endl;
    }
    (*cnt)++;
  }

  void External::resolve_external( ) {
    string src;
    try {
      src = AbstractElement::src();
      cerr << "try to resolve: " << src << endl;
      int cnt = 0;
      xmlSetStructuredErrorFunc( &cnt, (xmlStructuredErrorFunc)error_sink );
      xmlDoc *extdoc = xmlReadFile( src.c_str(), 0, XML_PARSE_NSCLEAN|XML_PARSE_HUGE );
      if ( extdoc ) {
	xmlNode *root = xmlDocGetRootElement( extdoc );
	xmlNode *p = root->children;
	while ( p ) {
	  if ( p->type == XML_ELEMENT_NODE ) {
	    string tag = Name( p );
	    if ( tag == "text" ) {
	      const string bogus_id = "Arglebargleglop-glyf";
	      FoliaElement *par = parent();
	      KWargs args = par->collectAttributes();
	      args["xml:id"] = bogus_id;
	      Text *tmp = new Text( args, doc() );
	      tmp->AbstractElement::parseXml( p );
	      FoliaElement *old = par->replace( this, tmp->index(0) );
	      doc()->del_doc_index( tmp, bogus_id );
	      tmp->remove( (size_t)0, false );
	      delete tmp;
	      delete old;
	    }
	  }
	  p = p->next;
	}
	xmlFreeDoc( extdoc );
      }
      else {
	throw XmlError( "resolving external " + src + " failed" );
      }
    }
    catch ( const exception& e ) {
      throw XmlError( "resolving external " + src + " failed: "
		      + e.what() );
    }
  }

  FoliaElement* External::parseXml( const xmlNode *node ) {
    KWargs att = getAttributes( node );
    setAttributes( att );
    if ( _include ) {
      doc()->addExternal( this );
    }
    return this;
  }

  KWargs External::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( _include ) {
      atts["include"] = "yes";
    }
    return atts;
  }

  void External::setAttributes( const KWargs& kwargsin ) {
    KWargs kwargs = kwargsin;
    auto it = kwargs.find( "include" );
    if ( it != kwargs.end() ) {
      _include = TiCC::stringTo<bool>( it->second );
      kwargs.erase( it );
    }
    AbstractElement::setAttributes(kwargs);
  }

  void Note::setAttributes( const KWargs& args ) {
    KWargs a = args;
    auto it = a.find( "id" );
    if ( it != a.end() ) {
      refId = it->second;
      a.erase( it );
    }
    AbstractElement::setAttributes( a );
  }

  KWargs Reference::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( !refId.empty() ){
      atts["id"] = refId;
    }
    if ( !ref_type.empty() ){
      atts["type"] = ref_type;
    }
    if ( !_format.empty() && _format != "text/folia+xml" ) {
      atts["format"] = _format;
    }
    return atts;
  }

  void Reference::setAttributes( const KWargs& argsin ) {
    KWargs args = argsin;
    auto it = args.find( "id" );
    if ( it != args.end() ) {
      refId = it->second;
      args.erase( it );
    }
    it = args.find( "type" );
    if ( it != args.end() ) {
      // if ( it->second != "simple" ){
	ref_type = it->second;
	//      }
      args.erase( it );
    }
    it = args.find( "format" );
    if ( it != args.end() ) {
      _format = it->second;
      args.erase( it );
    }
    AbstractElement::setAttributes(args);
  }

  KWargs TextMarkupReference::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( !refId.empty() ){
      atts["id"] = refId;
    }
    if ( !ref_type.empty() ){
      atts["type"] = ref_type;
    }
    if ( !_format.empty() && _format != "text/folia+xml" ) {
      atts["format"] = _format;
    }
    return atts;
  }

  void TextMarkupReference::setAttributes( const KWargs& argsin ) {
    KWargs args = argsin;
    auto it = args.find( "id" );
    if ( it != args.end() ) {
      refId = it->second;
      args.erase( it );
    }
    it = args.find( "type" );
    if ( it != args.end() ) {
      ref_type = it->second;
      args.erase( it );
    }
    it = args.find( "format" );
    if ( it != args.end() ) {
      _format = it->second;
      args.erase( it );
    }
    AbstractElement::setAttributes(args);
  }

  xmlNode *XmlComment::xml( bool, bool ) const {
    return xmlNewComment( (const xmlChar*)_value.c_str() );
  }

  FoliaElement* XmlComment::parseXml( const xmlNode *node ) {
    if ( node->content ) {
      _value = (const char*)node->content;
    }
    return this;
  }

  KWargs Suggestion::collectAttributes() const {
    KWargs atts = AbstractElement::collectAttributes();
    if ( !_split.empty() ) {
      atts["split"] = _split;
    }
    if ( !_merge.empty() ) {
      atts["merge"] = _merge;
    }
    return atts;
  }

  void Suggestion::setAttributes( const KWargs& kwargsin ) {
    KWargs kwargs = kwargsin;
    auto it = kwargs.find( "split" );
    if ( it != kwargs.end() ) {
      _split = it->second;
      kwargs.erase( it );
    }
    it = kwargs.find( "merge" );
    if ( it != kwargs.end() ) {
      _merge = it->second;
      kwargs.erase( it );
    }
    AbstractElement::setAttributes(kwargs);
  }


  void Feature::setAttributes( const KWargs& kwargs ) {
    //
    // Feature is special. So DON'T call ::setAttributes
    //
    auto it = kwargs.find( "subset" );
    if ( it == kwargs.end() ) {
      _subset = default_subset();
      if ( _subset.empty() ){
	throw ValueError("subset attribute is required for " + classname() );
      }
    }
    else {
      if ( it->second.empty() ) {
	throw ValueError("subset attribute may never be empty: " + classname() );
      }
      _subset = it->second;
    }
    it = kwargs.find( "class" );
    if ( it == kwargs.end() ) {
      throw ValueError("class attribute is required for " + classname() );
    }
    if ( it->second.empty() ) {
      throw ValueError("class attribute may never be empty: " + classname() );
    }
    update_cls( it->second );
  }

  KWargs Feature::collectAttributes() const {
    KWargs attribs = AbstractElement::collectAttributes();
    attribs["subset"] = _subset;
    return attribs;
  }

  vector<string> AbstractElement::feats( const string& s ) const {
    //    return all classes of the given subset
    vector<string> result;
    for ( const auto& el : data() ) {
      if ( el->isSubClass( Feature_t ) &&
	   el->subset() == s ) {
	result.push_back( el->cls() );
      }
    }
    return result;
  }

  const string AbstractElement::feat( const string& s ) const {
    //    return the fist class of the given subset
    for ( const auto& el : data() ) {
      if ( el->isSubClass( Feature_t ) &&
	   el->subset() == s ) {
	return el->cls();
      }
    }
    return "";
  }

  ForeignMetaData::~ForeignMetaData(){
    for ( const auto& it : foreigners ){
      delete it;
    }
  }

  void ForeignMetaData::add_foreign( const xmlNode *node ){
    ForeignData *fd = new ForeignData();
    fd->set_data( node );
    foreigners.push_back( fd );
  }

  ForeignData::~ForeignData(){
    xmlFreeNode( _foreign_data );
  }

  FoliaElement* ForeignData::parseXml( const xmlNode *node ){
    set_data( node );
    return this;
  }

  xmlNode *ForeignData::xml( bool, bool ) const {
    return get_data();
  }

  void ForeignData::set_data( const xmlNode *node ){
    xmlNode *p = (xmlNode *)node->children;
    while ( p ){
      string pref;
      string ns = getNS( p, pref );
      if ( ns == NSFOLIA ){
	throw XmlError( "ForeignData MAY NOT be in the FoLiA namespace" );
      }
      p = p->next;
    }
    _foreign_data = xmlCopyNode( (xmlNode*)node, 1 );
  }

  void clean_ns( xmlNode *node, const string& ns ){
    xmlNs *p = node->nsDef;
    xmlNs *prev = 0;
    while ( p ){
      string val = (char *)p->href;
      if ( val == ns ){
	if ( prev ){
	  prev->next = p->next;
	}
	else {
	  node->nsDef = p->next;
	}
	return;
      }
      prev = p;
      p = p->next;
    }
  }

  xmlNode* ForeignData::get_data() const {
    xmlNode *result = xmlCopyNode(_foreign_data, 1 );
    clean_ns( result, NSFOLIA ); // Sanity: remove FoLiA namespace defs, if any
    return result;
  }

  const MetaData* AbstractElement::get_metadata() const {
    // Get the metadata that applies to this element,
    // automatically inherited from parent elements
    if ( !_metadata.empty() && doc() ){
      return doc()->get_submetadata(_metadata);
    }
    else if ( parent() ){
      return parent()->get_metadata();
    }
    else {
      return 0;
    }
  }

  const string AbstractElement::get_metadata( const string& key ) const {
    // Get the metadata that applies to this element,
    // automatically inherited from parent elements
    if ( !_metadata.empty() && doc() ){
      const MetaData *what = doc()->get_submetadata(_metadata);
      if ( what && what->datatype() == "NativeMetaData" && !key.empty() ){
	return what->get_val( key );
      }
      return "";
    }
    else if ( parent() ){
      return parent()->get_metadata( key );
    }
    else {
      return "";
    }
  }

  KWargs AbstractTextMarkup::collectAttributes() const {
    KWargs attribs = AbstractElement::collectAttributes();
    if ( !idref.empty() ) {
      attribs["id"] = idref;
    }
    return attribs;
  }

  void AbstractTextMarkup::setAttributes( const KWargs& atts ) {
    KWargs args = atts;
    auto it = args.find( "id" );
    if ( it != args.end() ) {
      auto it2 = args.find( "xml:id" );
      if ( it2 != args.end() ) {
	throw ValueError("Both 'id' and 'xml:id found for " + classname() );
      }
      idref = it->second;
      args.erase( it );
    }
    it = args.find( "text" );
    if ( it != args.end() ) {
      XmlText *txt = new XmlText();
      txt->setvalue( it->second );
      append(txt);
      args.erase( it );
    }
    AbstractElement::setAttributes( args );
  }

  KWargs TextMarkupCorrection::collectAttributes() const {
    KWargs attribs = AbstractTextMarkup::collectAttributes();
    if ( !_original.empty() ) {
      attribs["original"] = _original;
    }
    return attribs;
  }

  void TextMarkupCorrection::setAttributes( const KWargs& args ) {
    KWargs argl = args;
    auto it = argl.find( "id" );
    if ( it != argl.end() ) {
      idref = it->second;
      argl.erase( it );
    }
    it = argl.find( "original" );
    if ( it != argl.end() ) {
      _original = it->second;
      argl.erase( it );
    }
    AbstractElement::setAttributes( argl );
  }

  const UnicodeString TextMarkupCorrection::private_text( const string& cls,
							  bool ret,
							  bool strict,
							  bool hidden ) const{
    if ( cls == "original" ) {
      return TiCC::UnicodeFromUTF8(_original);
    }
    return AbstractElement::private_text( cls, ret, strict, hidden );
  }

  const FoliaElement* AbstractTextMarkup::resolveid() const {
    if ( idref.empty() || !doc() ) {
      return this;
    }
    else {
      return doc()->index(idref);
    }
  }

  void TextContent::init() {
    _offset = -1;
  }

  void PhonContent::init() {
    _offset = -1;
  }

  void Linebreak::init() {
    _newpage = false;
  }

  void Relation::init() {
    _format = "text/folia+xml";
  }

  void Reference::init() {
    _format = "text/folia+xml";
  }

  void TextMarkupReference::init() {
    _format = "text/folia+xml";
  }

  void ForeignData::init() {
    _foreign_data = 0;
  }

} // namespace folia
