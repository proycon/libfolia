#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <stdexcept>
#include <algorithm>
#include "folia/folia.h"

using namespace std;

UnicodeString UTF8ToUnicode( const string& s ){
  return UnicodeString( s.c_str(), s.length(), "UTF-8" );
}

string UnicodeToUTF8( const UnicodeString& s ){
  string result;
  int len = s.length();
  if ( len > 0 ){
    char *buf = new char[len*6+1];
    s.extract( 0, len, buf, len*6, "UTF-8" );
    result = buf;
    delete [] buf;
  }
  return result;
}

double toDouble( const string& s ){
  char *end;
  double res = strtod( s.c_str(), &end );
  return res;
}

string toString( const double d ){
  stringstream dummy;
  if ( !( dummy << d ) ) {
    throw( runtime_error( "conversion to string failed" ) );
  }
  return dummy.str();
}

string toString( const AnnotationType::AnnotationType& at ){
  string result;
  switch ( at ) {
  case AnnotationType::NO_ANN:
    result = "None";
    break; 
  case AnnotationType::TOKEN:
    result = "TOKEN";
    break; 
  case AnnotationType::DIVISION:
    result = "DIVISION";
    break; 
  case AnnotationType::POS: 
    result = "POS";
    break; 
  case AnnotationType::LEMMA:
    result = "LEMMA";
    break; 
  case AnnotationType::DOMAIN:
    result = "DOMAIN";
    break; 
  case AnnotationType::SENSE: 
    result = "SENSE";
    break; 
  case AnnotationType::SYNTAX: 
    result = "SYNTAX";
    break;
  case AnnotationType::CHUNKING: 
    result = "CHUNKING";
    break; 
  case AnnotationType::ENTITY: 
    result = "ENTITY";
    break;
  case AnnotationType::CORRECTION: 
    result = "CORRECTION";
    break;
  case AnnotationType::ERRORDETECTION: 
    result = "ERRORDETECTION";
    break;
  case AnnotationType::ALTERNATIVE: 
    result = "ALTERNATIVE";
    break; 
  case AnnotationType::PHON:
    result = "PHON";
    break;
  case AnnotationType::SUBJECTIVITY:
    result = "SUBJECTIVITY";
    break;
  case AnnotationType::MORPHOLOGICAL:
    result = "MORPHOLOGICAL";
    break;
  default:
    throw ValueError( " unknown translation for annotation" + toString(int(at)) );
  };
  return result;
}

string toString( const TextCorrectionLevel l ){
  string result;
  switch ( l ) {
  case NOCORR:
    result = "NONE";
    break; 
  case CORRECTED:
    result = "CORRECTED";
    break; 
  case UNCORRECTED:
    result = "UNCORRECTED";
    break; 
  case INLINE:
    result = "INLINE";
    break; 
  default:
    throw ValueError( " unknown translation for TextCorrectionLevel " + toString(int(l)) );
  }
  return result;
}

TextCorrectionLevel stringToTCL( const string& lev ){
  TextCorrectionLevel result;
  if ( lev == "NONE" )
    result = NOCORR;
  else if ( lev == "CORRECTED" || lev == "yes" )
    result = CORRECTED;
  else if ( lev == "UNCORRECTED" || lev == "no" )
    result = UNCORRECTED;
  else if ( lev == "INLINE" || lev == "inline" )
    result = INLINE;
  else
    throw ValueError( " unknown TextCorrectionLevel " + lev );
  return result;
}

int to_lower( const int& i ){ return tolower(i); }
int to_upper( const int& i ){ return toupper(i); }

string lowercase( const string& in ){
  string s = in;
  transform( s.begin(), s.end(), s.begin(), to_lower );
  return s;
}

string uppercase( const string& in ){
  string s = in;
  transform( s.begin(), s.end(), s.begin(), to_upper );
  return s;
}

AnnotatorType stringToANT( const string& s ){
  string at = uppercase( s );
  if ( at == "AUTO" )
    return AUTO;
  else if ( at == "MANUAL" )
    return MANUAL;
  else
    return UNDEFINED;
}

AnnotationType::AnnotationType stringToAT( const string& s ){
  string at = uppercase( s );
  if ( at == "TOKEN" )
    return AnnotationType::TOKEN;
  if ( at == "DIVISION" )
    return AnnotationType::DIVISION;
  if ( at == "POS" )
    return AnnotationType::POS;
  if ( at == "LEMMA" )
    return AnnotationType::LEMMA;
  if ( at == "DOMAIN" )
    return AnnotationType::DOMAIN;
  if ( at == "SENSE" )
    return AnnotationType::SENSE;
  if ( at == "SYNTAX" )
    return AnnotationType::SYNTAX;
  if ( at == "CHUNKING" )
    return AnnotationType::CHUNKING;
  if ( at == "ENTITY" )
    return AnnotationType::ENTITY;
  if ( at == "CORRECTION" )
    return AnnotationType::CORRECTION; 
  if ( at == "ERRORDETECTION" )
    return AnnotationType::ERRORDETECTION; 
  if ( at == "ALTERNATIVE" )
    return AnnotationType::ALTERNATIVE;
  if ( at == "PHON" )
    return AnnotationType::PHON;
  if ( at == "SUBJECTIVITY" )
    return AnnotationType::SUBJECTIVITY;
  if ( at == "MORPHOLOGICAL" )
    return AnnotationType::MORPHOLOGICAL;
  throw ValueError( " unknown translation for attribute: " + at );
}

string toString( const ElementType& et ) {
  string result;
  switch( et ){
  case BASE: result = "BASE"; break;
  case Text_t: result = "text"; break;
  case TextContent_t: result = "t"; break;
  case Word_t: result = "word"; break;
  case PlaceHolder_t: result = "placeholder"; break;
  case Sentence_t: result = "s"; break;
  case Paragraph_t: result = "p"; break;
  case Division_t: result = "div"; break;
  case Head_t: result = "head"; break;
  case Description_t: result = "desc"; break;
  case Gap_t: result = "gap"; break;
  case Content_t: result = "content"; break;
  case List_t: result = "list"; break;
  case ListItem_t: result = "listitem"; break;
  case Figure_t: result = "figure"; break;
  case Quote_t: result = "quote"; break;
  case Pos_t: result = "pos"; break;
  case Current_t: result = "current"; break;
  case New_t: result = "new"; break;
  case Original_t: result = "original"; break;
  case Suggestion_t: result = "suggestion"; break;
  case Lemma_t: result = "lemma"; break;
  case Domain_t: result = "DOMAIN"; break; 
  case Sense_t: result = "SENSE"; break; 
  case Morphology_t: result = "MORPHOLOGY"; break;
  case Morpheme_t: result = "MORPHEME"; break;
  case Correction_t: result = "CORRECTION"; break;
  case ErrorDetection_t: result = "ERRORDETECTION"; break;
  case Annolay_t: result = "ANNOTATIONLAYER"; break; 
  case Syntax_t: result = "SYNTAX"; break; 
  case SyntacticUnit_t: result = "su"; break; 
  case WordReference_t: result = "wref"; break; 
  case Chunking_t: result = "CHUNKING"; break; 
  case Entities_t: result = "ENTITIES"; break;
  case Alternative_t: result = "ALTERNATIVE"; break; 
  case AltLayers_t: result = "ALTLAYERS"; break;
  case Feature_t: result = "FEATURE"; break;
  default:
    result = "Unknown Elementtype " + toString( int(et) );
  }
  return result;
}

AbstractElement *AbstractElement::createElement( Document *doc, 
						 const string& tag ){
  //factory;
  if ( tag == "FoLiA" ){
    return new FoLiA( doc );
  }
  if ( tag == "DCOI" ){
    return new DCOI( doc );
  }
  if ( tag == "text" ){
    return new Text( doc );
  }
  if ( tag == "s" ){
    return new Sentence( doc );
  }
  if ( tag == "t" ){
    return new TextContent( doc );
  }
  if ( tag == "w" ){
    return new Word( doc );
  }
  if ( tag == "p" ){
    return new Paragraph( doc );
  }
  if ( tag == "new" ){
    return new NewElement( doc );
  }
  if ( tag == "original" ){
    return new Original( doc );
  }
  if ( tag == "suggestion" ){
    return new Suggestion( doc );
  }
  if ( tag == "head" ){
    return new Head( doc );
  }
  if ( tag == "desc" ){
    return new Description( doc );
  }
  if ( tag == "gap" ){
    return new Gap( doc );
  }
  if ( tag == "content" ){
    return new Content( doc );
  }
  if ( tag == "div" ){
    return new Division( doc );
  }
  if ( tag == "lemma" ){
    return new LemmaAnnotation( doc );
  }
  if ( tag == "pos" ){
    return new PosAnnotation( doc );
  }
  if ( tag == "syntax" ){
    return new SyntaxLayer( doc );
  }
  if ( tag == "su" ){
    return new SyntacticUnit( doc );
  }
  if ( tag == "wref" ){
    return new WordReference( doc );
  }
  if ( tag == "correction" ){
    return new Correction( doc );
  }
  if ( tag == "errordetection" ){
    return new ErrorDetection( doc );
  }
  if ( tag == "morphology" ){
    return new MorphologyLayer( doc );
  }
  if ( tag == "morpheme" ){
    return new Morpheme( doc );
  }
  if ( tag == "feat" ){
    return new Feature( doc );
  }
  else {
    //    throw runtime_error( "unknown tag <" + tag + ">" );
    cerr << "unknown tag <" << tag << ">" << endl;
  }
  return 0;
}


string compress( const string& s ){
  // remove leading and trailing spaces from a string
  string result;
  if ( !s.empty() ){
    string::const_iterator b_it = s.begin();
    while ( b_it != s.end() && isspace( *b_it ) ) ++b_it;
    string::const_iterator e_it = s.end();
    --e_it;
    while ( e_it != s.begin() && isspace( *e_it ) ) --e_it;
    if ( b_it <= e_it )
      result = string( b_it, e_it+1 );
  }
  return result;
}

size_t split_at( const string& src, vector<string>& results, 
		 const string& sep ){
  // split a string into substrings, using seps as seperator
  // silently skip empty entries (e.g. when two or more seperators co-incide)
  // also purges trailing and leading spaces
  results.clear();
  string::size_type pos = 0;
  string res;
  while ( pos != string::npos ){
    string::size_type p = src.find( sep, pos );
    if ( p == string::npos ){
      res = src.substr( pos );
      pos = p;
    }
    else {
      res = src.substr( pos, p - pos );
      pos = p + sep.length();
    }
    res = compress( res );
    if ( !res.empty() )
      results.push_back( res );
  }
  return results.size();
}

KWargs getArgs( const std::string& s ){
  KWargs result;
  bool quoted = false;
  bool parseatt = true;
  vector<string> parts;
  string att;
  string val;
  //  cerr << "getArgs \\" << s << "\\" << endl;

  for ( size_t i=0; i < s.size(); ++i ){
    //    cerr << "bekijk " << s[i] << endl;
    //    cerr << "quoted = " << (quoted?"YES":"NO") << " parseatt = " << (parseatt?"YES":"NO") << endl;
    if ( s[i] == '\'' ){
      if ( quoted ){
	if ( att.empty() || val.empty() )
	  throw ArgsError( s + ", (''?)" );
	result[att] = val;
	//	cerr << "added " << att << "='" << val << "'" << endl;
	att.clear();
	val.clear();
	quoted = false;
      }
      else {
	quoted = true;
      }
    }
    else if ( s[i] == '=' ) {
      if ( parseatt ){
	parseatt = false;
      }
      else if ( quoted )
	val += s[i];
      else
	throw ArgsError( s + ", stray '='?" );
    }
    else if ( s[i] == ',' ){
      if ( quoted )
	val += s[i];
      else if ( !parseatt ){
	parseatt = true;
      }
      else
	throw ArgsError( s + ", stray '='?" );
    }
    else if ( s[i] == ' ' ){
      if ( quoted )
	val += s[i];
    }
    else if ( parseatt )
      att += s[i];
    else
      val += s[i];
    // cerr << "att = '" << att << "'" << endl;
    // cerr << "val = '" << val << "'" << endl;
  }
  if ( quoted )
    throw ArgsError( s + ", unbalanced '?" );
  return result;
}

std::string toString( const KWargs& args ){
  string result;
  KWargs::const_iterator it = args.begin();
  while ( it != args.end() ){
    result += it->first + "='" + it->second + "'";
    ++it;
    if ( it != args.end() )
      result += ",";
  }
  return result;

}
string Name( xmlNode *node ){
  string result;
  if ( node ){
    result = (char *)node->name;
  }
  return result;
}

string XmlContent( xmlNode *node ){
  string result;
  if ( node ){
    xmlChar *tmp = xmlNodeListGetString( node->doc, node->children, 1 );
    if ( tmp ){
      result = string( (char *)tmp );
      xmlFree( tmp );
    }
  }
  return result;
}

string getNS( xmlNode *node, string& prefix ){
  string result;
  prefix = "";
  xmlNs *p = node->ns;
  if ( p ){
    if ( p->prefix ){
      prefix = (char *)p->prefix;
    }
    result = (char *)p->href;
  }
  return result;
}

map<string,string> getNSlist( xmlNode *node ){
  map<string,string> result;
  xmlNs *p = node->ns;
  while ( p ){
    string pre;
    string val;
    if ( p->prefix ){
      pre = (char *)p->prefix;
    }
    val = (char *)p->href;
    result[pre] = val;
    p = p->next;
  }
  return result;
}

list<xmlNode*> FindLocal( xmlXPathContext* ctxt, 
			  const string& xpath ){
  list<xmlNode*> nodes;
  xmlXPathObject* result = xmlXPathEval((xmlChar*)xpath.c_str(), ctxt);
  if ( result ){
    if (result->type != XPATH_NODESET) {
      xmlXPathFreeObject(result);
      throw runtime_error( "sorry, only nodeset result types supported for now." );
      return nodes;
    }
    xmlNodeSet* nodeset = result->nodesetval;
    if ( nodeset ){
      for (int i = 0; i != nodeset->nodeNr; ++i)
	nodes.push_back(nodeset->nodeTab[i]);
    }
    else {
      throw( runtime_error( "FindLocal: Missing nodeset" ) );
    }
    xmlXPathFreeObject(result);
  }
  else {
    throw runtime_error( "Invalid Xpath: '" + xpath + "'" );
  }
  return nodes;
}

list<xmlNode*> FindNodes( xmlNode* node,
			  const string& xpath ){
  xmlXPathContext* ctxt = xmlXPathNewContext( node->doc );
  ctxt->node = xmlDocGetRootElement( node->doc );
  ctxt->namespaces = xmlGetNsList( node->doc, ctxt->node );
  ctxt->nsNr = 0;
  if (ctxt->namespaces != 0 ) {
    while (ctxt->namespaces[ctxt->nsNr] != 0 ){
      ctxt->nsNr++;
    }
  }
  list<xmlNode*> nodes = FindLocal( ctxt, xpath );
  if (ctxt->namespaces != NULL)
    xmlFree(ctxt->namespaces);
  xmlXPathFreeContext(ctxt);
  return nodes;
}

xmlNode *xPath( xmlNode *node, const string& xpath ){
  // try to find a path, but it may not be there...
  // if there are more, just return the first

  list<xmlNode*> srch = FindNodes( node, xpath );
  xmlNode *result = 0;
  if ( srch.size() != 0 ){
    result = srch.front();
  }
  return result;
}

