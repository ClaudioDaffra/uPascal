
#include "global.h"
#include "symTable.h"

static FILE* pFileOutputST      ; // symTable.c
static char* fileNameOutputST   ;  

extern global_t g ;

stScope_t stScope ;

// init stNameSpace

#define $CD gcWcsDup((wchar_t*)L"\\")

// .............................................. stInitSymTable

void stInitSymTable(void)
{
	pFileOutputST 		= NULL ;
	fileNameOutputST 	= NULL ;

	if ( g.fDebug ) 
	{
		fileNameOutputST = g.makeFileWithNewExt( g.fileInputName , ".symTable" ) ; // .............file symTable
		stdFileWOpen ( &pFileOutputST , fileNameOutputST , "w+","ccs=UTF-8") ;
		
		fwprintf ( pFileOutputST , L"\n# symTable   -> initializing ...\n\n" );
	}

	stScope = stScopeGlobal 			;	// parti dalle dichiarzioni globali
	
	vectorNew(stNameSpace,16) 			;	// definisci un iniziale spazio di 16 nomi in cascata
	vectorPushBack(stNameSpace,$CD) 	;	// inserisce il primo spazio dei nomi
	
    // GNU GCC 9.2 : lib/hmap.c:299:39: warning: 
    // ISO C forbids conversion of object pointer to function pointer type [-Wpedantic] C11
    #ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #endif	
    
	mapST = whmapDef ( gcComparepWStrC ); 	// inizializza la funzione di ricerca nomi per la tabella hash
	
	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}

// .............................................. stDeInitSymTable

void stDeInitSymTable(void)
{
	if ( g.fDebug ) 
	{
		fwprintf ( pFileOutputST , L"\n# symTable   -> denitialized . \n\n" );
		if (  pFileOutputST != NULL ) fclose ( pFileOutputST ) ;
	}
}

// .............................................. st Get NameSpace(level) + id

// Questa funzione genera un nome composto da : NAMESPACE + ID
// size_t 	level	: livelli da utilizzare per comporre il name space
//					: 0 tutti i livelli
// wchar_t*	id		: nome attuale identificatore
//
// ex NS \f1
//	  	stGet_nsid(0,id)
//		\id
//		\f1id

wchar_t* stGet_nsid(size_t level,wchar_t* id) // ns(l)+id
{
	//if ( fDebug ) fwprintf ( pFileOutputST , L"# symTable   -> stGet_nsid " );
	
	size_t MAXBUFFER = 4096 ;
	wchar_t* temp = gcMalloc( sizeof(wchar_t) * MAXBUFFER ) ;
	temp[0] = 0 ;
	size_t k=0;
	
	// 0 		:	from 0 to vectorSize(stNameSpace)
	// 1 - x 	: 	from 0 to level
	if ( level==0 ) level=vectorSize(stNameSpace) ;
	
	if ( level<=vectorSize(stNameSpace) )
		for (  size_t i = 0 ; i<level; i++ )
		{
			for (  size_t j = 0 ; vectorAt(stNameSpace,i)[j] ; j++ )
			{
				if ( k>=MAXBUFFER-1 ) break ;
				temp[k++]=vectorAt(stNameSpace,i)[j];
			}
		}
		
	if (id!=NULL)
		for (  size_t j = 0 ; wcslen(id) ; j++ )
		{
			if ( k>=MAXBUFFER-1 ) break ;
			temp[k++]=id[j];
		}
	
	temp[k]=0;
	
	//if ( fDebug ) fwprintf ( pFileOutputST , L"[%ls].\n",temp );

	return gcWcsDup(temp) ;
}

// .............................................. show map

void stShowMap(void)
{
    for ( whmapIt_t* it=whmapBegin(mapST) ; it != whmapEnd(mapST) ; whmapNext(mapST,it) )  
	{
		fwprintf ( pFileOutputST , L"\n§§§ ((%ls,%p)) ", whmapData(it) , (void*)whmapFind(mapST, whmapData(it) ) );
	}
	fwprintf ( pFileOutputST,  L"\n");
}

// .............................................. find simbol in map

psymTable_t stFindIDinMap(wchar_t* id) 
{
	psymTable_t pret=NULL;
	// #1 per tutti i livelli da 0 a n del namespace + ID
	// cerca nella mappa
 
	size_t i=vectorSize(stNameSpace);
	do
	{
		i--;
		//fwprintf ( stdout , L"\nNAME SPACE::[%ls]",vectorAt(stNameSpace,i) ) ;
		//fwprintf ( stdout , L"\nFULL NAME ::[%ls]",stGet_nsid(i,id) 	) ;
		pret = whmapFind( mapST , stGet_nsid(i,id) ) ; // cerca il id nella mappa 
		if ( pret != NULL ) return pret ; // ritorna indice se l'ha trovato
		
	} while(i!=0);
 
	return pret ;
}

// .............................................. Make Sym Table and push in hasmap

psymTable_t stMakeSymTable(void)
{
	if ( g.fDebug ) fwprintf ( pFileOutputST , L"# symTable   -> stMakeSymTable ...\n" );
	
	psymTable_t pstNew   = NULL ; // new node Symbol Table
    
    pstNew = gcMalloc ( sizeof(struct symTable_s) ) ;
    if ( pstNew==NULL ) $scannerInternal ( malloc , outOfMemory , L"symTable.c" , L"stMakeNode") ;

	pstNew->scope	= stScope ;
 	pstNew->ns		= stGet_nsid(0,NULL) ;
	pstNew->id 		= NULL ;
	
	pstNew->kind 	= stKindNull ;
	pstNew->type 	= stTypeNull ;
	
	pstNew->nDim 	= 0 ;
	vectorNew( pstNew->aDim 	, 3 ) ;
	
	vectorNew( pstNew->member 	, 8 ) ;
	
	pstNew->size 	= 0 ;
	pstNew->offset 	= 0 ; 
	pstNew->address = NULL ; 
	pstNew->typeID  = NULL ;
	
	//pstNew->value.integer  = 0 ; // default value
	
    return pstNew ;
}

// .............................................. Debug Sym Table (node)

void stDebugSymTableNode(psymTable_t pst)
{
	if ( g.fDebug ) // typeID = struct name
	{
		fwprintf ( pFileOutputST , L"\n# DEBUG ST [%p] ->\n",pst );
		
		fwprintf ( pFileOutputST , L"# symTable->scope   [%d] :: "				,pst->scope );
		switch ( pst->scope )
		{
			case stScopeGlobal :  fwprintf ( pFileOutputST , L"[Global]"); break ;
			case stScopeLocal  :  fwprintf ( pFileOutputST , L"[Local]"); break ;
			default : fwprintf ( pFileOutputST , L"[??]"); break ;
		}
		fwprintf ( pFileOutputST , L"\n");
		
		fwprintf ( pFileOutputST , L"# symTable->ns      [%ls]\n"				,pst->ns 	 );
		fwprintf ( pFileOutputST , L"# symTable->id      [%ls]\n"				,pst->id 	 );
		
		fwprintf ( pFileOutputST , L"# symTable->kind    [%d] :: " 				,pst->kind   );
		switch ( pst->kind )
		{
			case stKindNull		: fwprintf ( pFileOutputST , L"[Null]"); 	 break ;
			case stKindConst	: fwprintf ( pFileOutputST , L"[Const]"); 	 break ;
			case stKindVar		: fwprintf ( pFileOutputST , L"[Var]"); 	 break ;
			case stKindArray	: fwprintf ( pFileOutputST , L"[Array]"); 	 break ;
			case stKindStruct	: fwprintf ( pFileOutputST , L"[Struct]"); 	 break ;
			case stKindFunction	: fwprintf ( pFileOutputST , L"[function]"); break ;
			default : fwprintf ( pFileOutputST , L"[??]"); break ;
		}
		fwprintf ( pFileOutputST , L"\n");
		
		fwprintf ( pFileOutputST , L"# symTable->type    [%d] :: " 				,pst->type  );
		switch ( pst->type )
		{
			case stTypeNull  	: fwprintf ( pFileOutputST , L"[stTypeNull]" 		); break ;
			case stTypeInteger  : fwprintf ( pFileOutputST , L"[stTypeInteger]"		); break ;
			case stTypeReal  	: fwprintf ( pFileOutputST , L"[stTypeReal]"		); break ;
			case stTypeID  		: fwprintf ( pFileOutputST , L"[stTypeID]" 			); break ;  
			case stTypeString   : fwprintf ( pFileOutputST , L"[stTypeString]"		); break ;
			case stTypeChar  	: fwprintf ( pFileOutputST , L"[stTypeChar]"		); break ;
			case stTypeByte  	: fwprintf ( pFileOutputST , L"[stTypeByte]"		); break ;
			default: fwprintf ( pFileOutputST , L"[??]"	); 	break ;
		} ;
		
		fwprintf ( pFileOutputST , L"\n# symTable->nDim    [%d] / " 			,pst->nDim  );
		for(size_t i=0;i<vectorSize(pst->aDim);i++) fwprintf ( pFileOutputST , L"[%d]" ,vectorAt(pst->aDim,i)  );
		fwprintf ( pFileOutputST , L"\n# symTable->member  [%d] / " ,vectorSize(pst->member)  );
		for(size_t i=0;i<vectorSize(pst->member);i++) fwprintf ( pFileOutputST , L"[%d]" ,(void*)vectorAt(pst->member,i)  );
		fwprintf ( pFileOutputST , L"\n# symTable->size    [%d]\n" 				,pst->size  );
		fwprintf ( pFileOutputST , L"# symTable->offset  [%d]\n" 				,pst->offset  );
		fwprintf ( pFileOutputST , L"# symTable->address [%d]\n" 				,pst->address  );
		fwprintf ( pFileOutputST , L"# symTable->typeID  [%ls]\n" 				,(pst->typeID == NULL) ? L"{NULL}" : pst->typeID );
		
		/*
		fwprintf ( pFileOutputST , L"# symTable->value   "  );
		switch ( pst->type )
		{
			case stTypeNull  	: fwprintf ( pFileOutputST , L"[stTypeNull]"						); break ;
			case stTypeInteger  : fwprintf ( pFileOutputST , L"[%ld]"		, pst->value.integer    ); break ;
			case stTypeReal  	: fwprintf ( pFileOutputST , L"[%lf]"		, pst->value.real		); break ;
			case stTypeID  		: fwprintf ( pFileOutputST , L"[STRUCT]" 							); break ;  
			case stTypeString   : fwprintf ( pFileOutputST , L"[%lf]"		, pst->value.string		); break ;
			case stTypeChar  	: fwprintf ( pFileOutputST , L"[%lc]"		, pst->value.character	); break ;
			case stTypeByte  	: fwprintf ( pFileOutputST , L"[%u]"		, pst->value.byte		); break ;
			default:
				fwprintf ( pFileOutputST , L"[??]"	); 
				break ;
		} ;
		*/
		
		fwprintf ( pFileOutputST , L"\n\n");
	}
}


#undef $CD



/**/


