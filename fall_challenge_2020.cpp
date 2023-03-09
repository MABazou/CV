#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
// #include <stdio.h>
// #include <string.h>
#include <cmath>
#include <sstream>
#include <list>

using namespace std;
const int MAXINGREDIENT = 10;
const int MAXTREEDEPTH = 3
;
const int TURNLEARN = 5;

/*---------- TODO ------------
- ma strategy peut/doit dépendre de l'état de mon opponent 
    - integrate my_player and opp_player in strategy
+ add m_repeatable     dans "CAST id times"
+ calculer combien de fois on l'utilise
+ a grimoire spel can be repeatable
+ est-ce que ma rectte objectif est toujours valable?
+ delete getOut1 getSubmission1
+ clean code and comment
*/

#define DEBUG

#ifdef DEBUG
#   define ENTER() do {cerr <<"D:ENTER --- "<<__FUNCTION__ << endl; } while(0)
#   define EXIT() do {cerr << "D:EXIT  --- "<<__FUNCTION__ << endl; } while(0)
#   define DBG(x) do {cerr <<"D:"<<__FUNCTION__<<": " << x << endl; } while(0)
#   define DBG_E(x) do {cerr <<"D:"<<__FUNCTION__<<": " << #x<<"="<<x << endl; } while(0)
#else 
#   define ENTER() 
#   define EXIT() 
#   define DBG(x) 
#   define DBG_E(x) 
#endif

enum eActionType {
    eLEARN,         // 0- Apprendre un nouveau sort dans le grimoire
    eCAST,          // 1- Lancer un de vos sorts
    eOPPONENT_CAST, // 2-
    eREST,          // 3- Récupérer pour rafraîchir tous les sorts déjà lancés
    eBREW,          // 4- Préparer une potion pour marquer des points
    eWAIT
};

/*! This is an enum class */
enum eSortType{
    eSortByPrice,
    eSortByLength
};

class Values;
class Feuille;
class Chemin;

eActionType convertString2ActionType(string* actionType){
    if(actionType->compare("BREW")==0){
        return eBREW;
    }else if(actionType->compare("CAST")==0){
        return eCAST;
    }else if(actionType->compare("OPPONENT_CAST")==0){
        return eOPPONENT_CAST;
    }else if(actionType->compare("LEARN")==0){
        return eLEARN;
    }else{
        return eREST;
    }
}
char convertActionType2Char(eActionType actionType){
    if(actionType==eBREW){
        // cerr<< 'B'<<endl;
        return 'B';
    }else if(actionType==eCAST){
        // cerr<< 'C'<<endl;
        return 'C';
    }else if(actionType==eOPPONENT_CAST){
        // cerr<< 'O'<<endl;
        return 'O';
    }else if(actionType==eLEARN){
        // cerr<< 'l'<<endl;
        return 'L';
    }else{
        // cerr<< 'R'<<endl;
        return 'R';
    }
}
string convertActionType2String(eActionType actionType){
    if(actionType==eBREW){
        return "BREW ";
    }else if(actionType==eCAST){
        return "CAST ";
    }else if(actionType==eOPPONENT_CAST){
        return "O";
    }else if(actionType==eLEARN){
        return "LEARN ";
    }else{
        return "REST ";
    }
}

// *****************************************************************************************
// ----------------- VALUES CLASS DEFINITION
// *****************************************************************************************
class Values : public vector<int>{
    public:  
    Values(){}

    Values(int val0, int val1, int val2, int val3){
        this->push_back(val0);
        this->push_back(val1);
        this->push_back(val2);
        this->push_back(val3);
    }

    Values &operator+=(Values const& unitA){
        at(0) += unitA[0];
        at(1) += unitA[1];
        at(2) += unitA[2];
        at(3) += unitA[3];
        return *this;
    }      
    Values operator+(Values const& unitB) const{
        Values r(*this);
        r.at(0) += unitB[0];
        r.at(1) += unitB[1];
        r.at(2) += unitB[2];
        r.at(3) += unitB[3];
        return r;
    }      

    bool isRealisable(Values deltas){
        return (at(0)+deltas[0]>=0 && at(1)+deltas[1]>=0 && at(2)+deltas[2]>=0 && at(3)+deltas[3]>=0);
    }
    
    int contrainteNbIngredients(){
        return at(0) + at(1)+ at(2) +at(3);
    }

    bool contrainteValPos(){
        return (at(0)>=0 && at(1)>=0 && at(2)>=0 && at(3)>=0);
    }

    bool isPossibleInventory(){
        return (contrainteNbIngredients()<=MAXINGREDIENT && contrainteValPos());
    }

    void debugValues(){
        string s = to_string(at(0))+" "+to_string(at(1))+" "+to_string(at(2))+" "+to_string(at(3));
        DBG(s);
    }  

    bool produceOnly(){
        return (at(0)>=0 && at(1)>=0 && at(2)>=0 && at(3)>=0);
    }

    int evalPrice(){
        return at(0)+at(1)*2+at(2)*3+at(3)*4;
    }  
};

// *****************************************************************************************
// ----------------- VECVALUES CLASS DEFINITION
// *****************************************************************************************
class VecValues : public vector<Values>{
    public:
    void debug(){
        ENTER();
        for(int i=0; i<size(); i++){
            at(i).debugValues();
        }
    } 
};

// *****************************************************************************************
// ----------------- RECIPE CLASS DEFINITION
// *****************************************************************************************
class Recipe : public Values{
    public :
       int m_actionId;
       eActionType m_actionType;
       int m_price;
       bool m_castable;         // 1 si ceci est un sort d'un joueur qui n'est pas épuisé, 0 sinon.
       int m_tomeIndex = 0;
       int m_taxCount = 0 ;
       bool m_repeatable = 0;   // 1 si ce sort est répétable, 0 sinon

    Recipe(){}   

    Recipe(int delta0, int delta1, int delta2, int delta3, int actionId, string* actionType, int price, bool castable):Values(delta0, delta1, delta2, delta3){
        m_actionId = actionId; 
        m_actionType = convertString2ActionType(actionType);
        m_price = price;
        m_castable = castable;
    }

    void setLeague3(int tomeIndex, int taxCount, bool repeatable){
        m_tomeIndex = tomeIndex;
        m_taxCount = taxCount;
        m_repeatable = repeatable;
    }

    void debug(){
        string s = to_string(m_actionId)+"\t"+to_string(m_actionType)+"|"+convertActionType2Char(m_actionType)+" \t "+to_string(m_price)+" "+to_string(m_castable)+" "+to_string(m_repeatable)+" |\t ";
        s += to_string(at(0))+" "+to_string(at(1))+" "+to_string(at(2))+" "+to_string(at(3));
        DBG(s);
    }    

    void setSpellUnused(){
        m_castable = true; // 1
    }
    void setSpellUsed(){
        m_castable = false; // 0        
    }
    
    bool haveSameIng(Recipe recip){
        return (at(0)==recip.at(0) && at(1)==recip.at(1) && at(2)==recip.at(2) && at(3)==recip.at(3));
    }
};

// *****************************************************************************************
// ----------------- RECIPES CLASS DEFINITION
// *****************************************************************************************
class Recipes : public vector<Recipe>{
    public:
        int m_sortType; // price!=0 -> commande 
        bool m_IsSortCroissant;

    void debug(){
        DBG("m_actId  m_eActType \t m_price  m_castable \t DELTAS");

        for(int i=0; i<size(); i++){
            at(i).debug();
        }
    }  

    bool operator() (const Recipe &unitA, const Recipe &unitB) { 
        float dA, dB;        
        switch(m_sortType){
            case eSortByPrice:
                dA = unitA.m_price;
                dB = unitB.m_price;
                return (dA>dB);
                // return m_IsSortCroissant?(dA<dB):(dA>dB);
                break;
                                
            default:
                break;
                
        }
    }
    void sortByPrice() {
        m_sortType=eSortByPrice;
        std::sort (begin(), end(), *this); 
    }    

    int getRecipe(int id, Recipe& recipe){
        int idres = filterByActionId(id, recipe);
        return idres;
        
    }    
    int getBrews(Recipes& recipes){
        filterByActiontype(eBREW, recipes);
        return recipes.size();
    }
    int getMySpells(Recipes& recipes){
        filterByActiontype(eCAST, recipes);
        return recipes.size();
    }
    int getOppSpells(Recipes& recipes){
        filterByActiontype(eOPPONENT_CAST, recipes);
        return recipes.size();
    }
    int getBookSpells(Recipes& recipes){
        filterByActiontype(eLEARN, recipes);
        for(int k=0; k<recipes.size(); k++){
            recipes[k].m_price = recipes[k].evalPrice()-k;
        } 
        return recipes.size();
    }

    int filterByActiontype(eActionType actType, Recipes& recipes){
        for(int i= 0; i<size(); i++){
            if(at(i).m_actionType==actType){
                recipes.push_back(at(i));
            }
        }
        return recipes.size();
    } 
    int filterByActionId(int id, Recipe& recipe){
        for(int i= 0; i<size(); i++){
            if(at(i).m_actionType==id){
                recipe = at(i);
                return i;
            }
        }
        return -1;
    }     

    void updateSpels2Unused(){ // toutes à 1
        for(int i= 0; i<size(); i++){
            at(i).setSpellUnused();
        }
    }

    int spellsAllUsed(){
        int res = 0;
        for(int i= 0; i<size(); i++){
            res += (int) at(i).m_castable;
        }
        return res==0;        
    }

    bool isInRecipes(int id){
        for(int i=0; i<size(); i++){
            if(id == at(i).m_actionId){
                return true;
            }
        }
        return false;
    } 


    int recipesHaveSameIng(int actID, Recipe& recip){
        // Recipe recip;
        int idRes = getRecipe(actID, recip);
        for(int i=0; i<size(); i++){
            if(idRes == i){
                continue;
            }else{
                // compare value
                if(at(i).haveSameIng(recip)){
                    if(at(i).m_price>recip.m_price){
                        DBG("SAME RECIPE MORE EXPENSIVE !! ------");
                        return at(i).m_actionId;
                    }
                }
            }
        }      
        return -1;  
    }    

};

// *****************************************************************************************
// ----------------- FEUILLE CLASS DEFINITION
// *****************************************************************************************
class Feuille : public Values{
    public:
    int m_id = -1;
    eActionType m_eActType;
    Feuille* m_parent;
    int m_repatable;
    list<Feuille> m_children;

    Feuille(){}
    Feuille(int id, Values val): Values(val){
        m_id = id; 
        m_eActType = eCAST;
        m_parent = NULL;
        m_repatable = 0;
    }
    Feuille(int id, Values val, Feuille *parent): Values(val){
        m_id = id; 
        m_eActType = eCAST;
        m_parent = parent;
        m_repatable = 0;
    }    
    Feuille(int id, Values val, eActionType actType, Feuille *parent): Values(val){
        m_id = id; 
        m_eActType = actType;
        m_parent = parent;
        m_repatable = 0;
    }

    void setMembers(int id, Values val, eActionType actType, Feuille *parent){
        m_id = id; 
        m_eActType = actType;
        m_parent = parent;
        this->push_back(val[0]);
        this->push_back(val[1]);
        this->push_back(val[2]);
        this->push_back(val[3]);
    }

    void increaseRepeatable(){
        m_repatable++;
    }

    void setParent(Feuille *parent){
        m_parent = parent;
    }
    void setVal(Values const& unitA){
        at(0) = unitA[0];
        at(1) = unitA[1];
        at(2) = unitA[2];
        at(3) = unitA[3];
    }    

    void debug(){
        string s = to_string(m_eActType)+"|"+convertActionType2Char(m_eActType) +"\t"+to_string(m_id)+" : "+to_string(at(0))+" "+to_string(at(1))+" "+to_string(at(2))+" "+to_string(at(3))+" \t S = "+to_string(contrainteNbIngredients());
        DBG(s);
    }

    void debugParent(){
        if(m_parent!=NULL){
            m_parent->debug();
        }else{
            DBG("m_parent == NULL");
        }        
    }

    void debugChildren(){
        for(auto it=m_children.begin(); it!=m_children.end(); it++){
            Feuille f = *it;
            if(f.m_id>=0){
                f.debug();                    
            }
        }
    }

    int brewsRealisable(Recipes brews, int& nbChildren, list<Chemin>& paths);

    int generateFeuillesV2(Recipes bels, Recipes brews, int& nbChildren, list<Chemin>& paths, int depth=0);
    int generateFeuilles(Recipes bels, Recipes brews, int& nbChildren, list<Chemin>& paths, int depth=0);

    int createListSpelsToBrew(Feuille&f, Chemin& idSpels);
    
    // int debugListSpels(list<Chemin>& idSpels);

};

// *****************************************************************************************
// ----------------- CHEMIN CLASS DEFINITION
// *****************************************************************************************
class Chemin : public list<Feuille>{
    public:
    int m_length;   // size to achieve a brew
    int m_price;     // price of a brew in rubees

    void setMembers(int length, int price){
        m_length = length;
        m_price = price;
    }
    void setMembers(int price){
        m_length = size();
        m_price = price;
    }    

    int debugChemin(){  
        // ENTER();
        // DBG_E(size());
        // DBG_E(m_length);
           
        // DBG("length/price : "+to_string(m_length)+" / "+to_string(m_price));
        string s = "p/l : "+to_string(m_price)+"/"+to_string(m_length)+"  =  "+ to_string((float)m_price/(float)m_length)+"  -  ";
        for(auto it=begin(); it!=end(); it++){
            Feuille k = *it;
            s += to_string(k.m_id)+" "+ convertActionType2Char(k.m_eActType)+" "+ to_string(k.m_repatable) +" | ";
        }           
        DBG(s);
        // EXIT();
        return size();
    }
};

// // *****************************************************************************************
// // ----------------- STRATEGY CLASS DEFINITION
// // *****************************************************************************************
// /// Class Strategy description.
// /** Detailed description. */
class Strategy{
    public:
        // int m_gameTour;         // tour de jeu
        Values m_myInv;         // inventaire
        Recipes m_mySpels;      // sorts
        Recipes m_brews;        // commandes
        Recipes m_grimSpels;    // commandes

    Strategy(Values inv, Recipes bels, Recipes brews, Recipes grim):m_myInv(inv),m_mySpels(bels), m_brews(brews), m_grimSpels(grim){}    

    void debugStrategy(){ 
        Feuille currentState(0,m_myInv);
        DBG("MY INV");
        currentState.debug();
        DBG("BREWS");
        m_brews.debug();
        DBG("MY SPELLS");
        m_mySpels.debug();
        DBG("GRIMOIRE");
        m_grimSpels.debug();
    }

    int debugListSpels(list<Chemin>& idSpels){
        ENTER();
        DBG_E(idSpels.size());
        if(idSpels.size()>0){        
            for(auto it=idSpels.begin(); it!=idSpels.end(); it++){
                Chemin k = *it;
                k.debugChemin();
            }             
        }
        EXIT();
        return idSpels.size();
    }
    
    bool compareChemins(int sortType, const Chemin &unitA, const Chemin &unitB) { 
        float dA, dB;        
        // switch(m_sortType){
        switch(sortType){
            case eSortByPrice:
                dA = unitA.m_price;
                dB = unitB.m_price;
                // return m_IsSortCroissant?(dA<dB):(dA>dB);
                return (dA>dB); // décroissant 
                break;
            
            case eSortByLength:
                dA = unitA.m_length;
                dB = unitB.m_length;
                return (dA<dB); // croissant 
                break;    
                                
            default:
                break;
                
        }
    }

    static bool comparePrice(const Chemin &unitA, const Chemin &unitB){
        float dA, dB;        
        dA = unitA.m_price;
        dB = unitB.m_price;
        // return m_IsSortCroissant?(dA<dB):(dA>dB);
        return (dA>dB); // décroissant 
    }
    static bool compareLength(const Chemin &unitA, const Chemin &unitB){
        float dA, dB;        
        dA = unitA.m_length;
        dB = unitB.m_length;
        return (dA<dB); // croissant 
    }
    static bool compareRatio(const Chemin &unitA, const Chemin &unitB){
        float dA, dB;        
        dA = (float) unitA.m_price/(float) unitA.m_length;
        dB = (float) unitB.m_price/(float) unitB.m_length;
        return (dA>dB); 
    }

    stringstream strategyApproche3(){
        ENTER();
        int nbRessources = max(m_myInv[0]-1,0);
        Recipes recips;
        stringstream out;
        bool pOnlyFind = false;

        for(int i = nbRessources-1; i>=0; i--){
            recips.push_back(m_grimSpels[i]);
            if(m_grimSpels[i].produceOnly()){
                Feuille res(m_grimSpels[i].m_actionId, m_grimSpels[i], m_grimSpels[i].m_actionType, NULL);
                #ifdef DEBUG                
                    out = getOut(res);
                #else
                    out = getOutSubmission(res);
                #endif 
                pOnlyFind = true;
            }
        }

        if(!pOnlyFind){
            recips.sortByPrice();
            // recips.debug();
            Feuille res(recips[0].m_actionId, recips[0], recips[0].m_actionType, NULL);

            #ifdef DEBUG                
                out = getOut(res);
            #else
                out = getOutSubmission(res);
            #endif     
        }

        EXIT(); 
        return out;
    }

    stringstream strategyApproche2(bool& done, bool& getNewGoal, Chemin& path){
        ENTER();
        stringstream out;
        list<Chemin> idSpels;
        Feuille currentState(0,m_myInv);

        DBG_E(getNewGoal);
        Recipes spells = m_mySpels;
        spells.insert(spells.begin(), m_grimSpels.begin(), m_grimSpels.end());
        // spells.debug();

        if(getNewGoal){            
            DBG("-----------  GENERATE TREE ");
            int nbChildren = 0;            
            // currentState.generateFeuilles(spells, m_brews, nbChildren, idSpels);
            currentState.generateFeuillesV2(spells, m_brews, nbChildren, idSpels);

            DBG("possible states with \t depth 0 ");
            DBG_E(nbChildren);
            DBG_E(idSpels.size());
            
            if(idSpels.size()>0){
                DBG("-----------  SORT chemins ");
                // idSpels.sort(compareLength);
                // idSpels.sort(comparePrice);
                idSpels.sort(compareRatio);
                // debugListSpels(idSpels);

                DBG("-----------  CHOOSE A PATH ");
                path = idSpels.front();  
                // out = getOut(path, getNewGoal);

                // getNewGoal = false;

                Feuille res = path.front();
                path.debugChemin();
                #ifdef DEBUG                
                    out = getOut(res);
                #else
                    out = getOutSubmission(res);
                #endif
                if(res.m_repatable>0){
                    path.pop_front();
                }
                DBG_E(path.size());  
                path.pop_front();
                DBG_E(path.size());

                // 
                done = true;
            }    
        }else{
            // ----------------------------------------------------------------
            // ------------------ 2nd approche --------------------------------
            // choose an objectif in the tree -> generateFeuilles()
            // check if this objectif is still available
            // go over the id spels list
            DBG("-----------  USE TREE LIST");
            if(path.size()>0){

                // out = getOut(path, getNewGoal);
                path.debugChemin();
                Feuille res = path.front();
                #ifdef DEBUG                
                    out = getOut(res);
                #else
                    out = getOutSubmission(res);
                #endif

                if(res.m_eActType==eLEARN){
                    getNewGoal = true;
                }                
                if(res.m_repatable>0){
                    path.pop_front();
                }
                path.pop_front();
            }
            done = true;
            DBG_E(path.size());
        }
        if(path.size()==0){
            getNewGoal = true;
        }

        EXIT();
        return out;
    }

    stringstream strategyApproche1(bool done = false){
        // Recipes grimSpels, Recipes brews, Recipes mySpells, Values my_inventori
        ENTER();
        stringstream out;
        //bool done = false;
        // Ai-je assez d'ingrédients pour réaliser une commande?
        // si oui (réaliser la commande)
        // sinon (Est-ce que je peux jeter un sort?)
        // si oui (choix du sort)
        // sinon (Est ce que je dois me reposer?)
        // il y a t il un sort dans le grimoire à apprendre?

        if(!done){
            for (int i = 0; i < m_grimSpels.size(); i++)
            {
                int actionID = m_grimSpels[i].m_actionId;
                if(m_myInv[0]>i ){            
                    if(m_grimSpels[i].produceOnly()){
                        // out << "LEARN " << actionID<< " produce only " << actionID;
                        out << "LEARN " << actionID;
                        done = true;
                        break;
                    }else if(i==0 && m_grimSpels[i].m_price>2){
                        // out << "LEARN " << actionID<< " first and interesting " << actionID;
                        out << "LEARN " << actionID;
                        done = true;
                        break;
                    }
                }
                // else if(my_inventori[0]>i && (grimSpels[i].m_price+grimSpels[i].m_taxCount)>2){
                //     out << "LEARN " << actionID<< " enougth ressource and interesting " << actionID;
                //     done = true;
                //     break;
                // }
            }
        }

        // Ai-je assez d'ingrédients pour réaliser une commande?
        if(!done){
            for(int i=0; i<m_brews.size(); i++){
                // si oui (réaliser la commande)
                int actionID = m_brews[i].m_actionId;
                if(m_myInv.isRealisable(m_brews[i])){
                    DBG("--- BREW Approche1");
                    // out << "BREW " << actionID << " BREW " << actionID;
                    out << "BREW " << actionID ;
                    done = true;
                    break;
                }
            }
        }  
        
        // sinon (Est-ce que je peux jeter un sort?)
        int nbSpellsUsed = 0;
        if(!done){
            for(int i=0; i<m_mySpels.size(); i++){
                if(m_mySpels[i].m_castable==1){
                    // si oui (choix du sort)
                    Values res = m_myInv + m_mySpels[i];
                    int actionID = m_mySpels[i].m_actionId;
                    if(res.isPossibleInventory()){
                        DBG("--- CAST Approche1");
                        // out << "CAST " << actionID << " CAST " << actionID;
                        out << "CAST " << actionID ;
                        done = true;
                        break;
                    }
                }else{
                    nbSpellsUsed++;
                }            
            }
        }

        // sinon (Est ce que je dois me reposer?)
        if(!done || nbSpellsUsed==m_mySpels.size()){
            DBG("--- REST Approche1");
            out << "REST ";
        }
        EXIT();
        return out;
    }

    stringstream getOut(Chemin& path, bool& getNewGoal){
        stringstream out;
        bool twice = false;
        path.debugChemin();

        Feuille res = path.front();
        int actionID = res.m_id;
        string action = convertActionType2String(res.m_eActType);
        // res.debug();

        if(path.size()>1){
            path.pop_front();
            Feuille resNext = path.front();
            if(res.m_id == resNext.m_id){
                twice = true;
            }

        }        
        if(res.m_eActType==eREST){
            out << "REST REST";
        }else{
            // out << action <<" "<< actionID << " "<< action << actionID;
            if(twice){
                out << action << actionID << " 2 "<< action << actionID;
            }else{
                out << action << actionID << " "<< action << actionID;
            }
        }
        if(res.m_eActType==eLEARN){
            getNewGoal = true;
        }

        DBG_E(path.size());  
        path.pop_front();
        return out;
    }  

    stringstream getOut(Feuille res){
        stringstream out;
        res.debug();
        int actionID = res.m_id;
        string action = convertActionType2String(res.m_eActType);

        if(res.m_eActType==eREST){
            out << "REST REST";
        }else{
            // out << action <<" "<< actionID << " "<< action << actionID;
            if(res.m_repatable>0){
                out << action << actionID << " "<< res.m_repatable << " "<< action << actionID;
            }else{
                out << action << actionID << " "<< action << actionID;
            }
        }
        return out;
    }
    stringstream getOutSubmission(Feuille res){
        stringstream out;
        if(res.m_eActType==eREST){
            out << "REST ";
        }else{
            out << convertActionType2String(res.m_eActType) <<" "<< res.m_id;
        }
        return out;
    }

};

class Player{
    public:
        int m_score;
        Values m_inventori;
        Recipes m_spells;


};

/*
*   Nouvelle version de generateFeuille
*/ 
int Feuille::generateFeuillesV2(Recipes belsIni, Recipes brews, int& nbChildren, list<Chemin>& paths, int depth){
    depth++;
    // DBG("---- depth "+to_string(depth)+": "+to_string(m_id)+" "+to_string(paths.size()));

    // test brews
    for(int k=0; k<brews.size(); k++){
        Recipe brew = brews[k];
        if(isRealisable(brew)){
            Chemin idSpelsF;
            // DBG("----------- BREW realisable");  
            // DBG("---- depth "+to_string(depth)+": "+to_string(m_id)+" nbChilden="+to_string(nbChildren));
            // brew.debug();
            // debugChildren();

            Feuille stateBrew(brew.m_actionId, *this+brew, eBREW, this);
            m_children.push_back(stateBrew);
            nbChildren++;          
            createListSpelsToBrew(stateBrew, idSpelsF);
            idSpelsF.setMembers(brew.m_price);
            paths.push_back(idSpelsF);
            // idSpelsF.debugChemin();
            return 0;
        }                        
    }

    if(m_id>=0 && depth<MAXTREEDEPTH){          
        // Pour chaque sort créer un enfant
        for(int i=0; i<belsIni.size(); i++){

            Values valsomme = *this + belsIni[i];
            Recipes bels = belsIni;                
            eActionType aType = bels[i].m_actionType;

            Feuille state(bels[i].m_actionId, valsomme, bels[i].m_actionType, this);
            // test eActionType
            if(aType==eCAST){
                // DBG("----- eCAST ");  
                if(bels[i].m_castable==0){
                    Feuille stateRest(200, *this, eREST, this);                     
                    bels.updateSpels2Unused();
                    m_children.push_back(stateRest);
                    nbChildren++;  
                    m_children.back().generateFeuillesV2(bels, brews, nbChildren, paths, depth);
                    state.setMembers(bels[i].m_actionId, valsomme, bels[i].m_actionType, &m_children.back()); 
                }

                if(bels[i].m_repeatable==1){
                    // DBG("----- m_repeatable ");
                    // // depth--;
                    // int nbR = 0;
                    // while(state.isPossibleInventory() && nbR<5){
                    while(state.isPossibleInventory()){
                        bels[i].setSpellUsed(); 
                        state.m_repatable++;
                        // DBG("------ augment repeat "+to_string(state.m_id)+": "+to_string(state.m_repatable));  
                        Values val = state + bels[i];
                        state.setVal(val);
                        m_children.push_back(state);
                        nbChildren++;
                        m_children.back().generateFeuillesV2(bels, brews, nbChildren, paths, depth);
                        // nbR++;
                    }

                }else{
                    // DBG("----- NOT m_repeatable ");
                    if(state.isPossibleInventory()){
                        // DBG("-- NOT R -- depth "+to_string(depth)+"\t "+to_string(state.m_id)+": "+to_string(state.m_repatable)+" "+to_string(paths.size()));  
                        bels[i].setSpellUsed(); 
                        // state.m_repatable = 0;
                        m_children.push_back(state);
                        nbChildren++;
                        m_children.back().generateFeuillesV2(bels, brews, nbChildren, paths, depth);
                    }else{
                        Feuille defaultState(-1, Values(-1,-1,-1,-1));    
                        m_children.push_back(defaultState);
                        // return 0;
                    } 
                }
                
            }else if(aType==eLEARN && i<=this->at(0)){
                Feuille state(bels[i].m_actionId, valsomme, bels[i].m_actionType, this);
               //  DBG("----- eLEARN ");  
                if(bels[i].produceOnly()){
                    m_children.push_back(state);
                    nbChildren++;
                }else if(bels[i].m_price - i + bels[i].m_taxCount >=2){
                    m_children.push_back(state);
                    nbChildren++;
                }                
            }

        }
    }

    return 0;
}



int Feuille::generateFeuilles(Recipes belsIni, Recipes brews, int& nbChildren, list<Chemin>& paths, int depth){
    //ENTER();
    depth++;
    //if(depth<=2){
    // if(m_id==100){
    //     DBG("---- depth "+to_string(depth)+": "+to_string(m_id));
    // }

    for(int k=0; k<brews.size(); k++){
        Recipe brew = brews[k];
        if(isRealisable(brew)){
            Chemin idSpelsF;
            // DBG("----------- BREW realisable");  
            Feuille stateBrew(brew.m_actionId, *this+brew, eBREW, this);
            m_children.push_back(stateBrew);
            nbChildren++;          
            createListSpelsToBrew(stateBrew, idSpelsF);
            idSpelsF.setMembers(brew.m_price);
            paths.push_back(idSpelsF);
            // idSpelsF.debugChemin();
            return 0;
        }                        
    }

    if(m_id>=0 && depth<MAXTREEDEPTH){          
        // Pour chaque sort créer un enfant
        for(int i=0; i<belsIni.size(); i++){

            Values valsomme = *this + belsIni[i];
            Recipes bels = belsIni;                
            eActionType aType = bels[i].m_actionType;
            Feuille state(bels[i].m_actionId, valsomme, bels[i].m_actionType, this);

            if(aType==eCAST){
                if(state.isPossibleInventory()){
                    Feuille stateRest(200, *this, eREST, this);   
                    if(bels[i].m_castable==0){      
                        bels.updateSpels2Unused();
                        m_children.push_back(stateRest);
                        nbChildren++;
                        state.setMembers(bels[i].m_actionId, valsomme, bels[i].m_actionType, &m_children.back());                    
                    }
                    if(bels[i].m_repeatable==0){
                        bels[i].setSpellUsed(); 

                        m_children.push_back(state);
                        nbChildren++;
                        m_children.back().generateFeuilles(bels, brews, nbChildren, paths, depth);
                    }else{

                        // DBG("---- depth "+to_string(depth)+": "+to_string(m_id)+" ------ is repeatable");
                        // DBG(to_string(m_id)+" ------ is repeatable");  
                        // todo - calculer combien de fois on l'utilise
                        state.m_repatable++;

                        if(m_parent!=NULL && this->m_repatable>0){
                            // DBG("----------- augment parent "+to_string(this->m_repatable));  
                            this->m_repatable++;
                        }

                        if(state.m_repatable==1){                                          
                            m_children.push_back(state);
                            nbChildren++;
                            m_children.back().generateFeuilles(bels, brews, nbChildren, paths, depth);
                        }
                    }                                                                                    
                    // m_children.push_back(state);
                    // nbChildren++;
                    // m_children.back().generateFeuilles(bels, brews, nbChildren, paths, depth);
                }
                else{
                    Feuille defaultState(-1, Values(-1,-1,-1,-1));    
                    m_children.push_back(defaultState);
                } 
            }else if(aType==eLEARN && i<=this->at(0)){
                // DBG("---- LEARN ?"); 
                if(bels[i].produceOnly()){
                    // DBG("---- LEARN --- produce only"); 
                    m_children.push_back(state);
                    nbChildren++;
                // }else if(i==0 && bels[i].m_price>=2){
                }else if(bels[i].m_price - i + bels[i].m_taxCount >=2){
                    // DBG("---- LEARN --- interesting"); 
                    m_children.push_back(state);
                    nbChildren++;
                }
            }
        }
        
        // DBG("possible states with \t depth "+to_string(depth)+" \t"+to_string(m_children.size()));
    }
    // if(depth==2){
    // DBG("---- depth "+to_string(depth)+": "+to_string(m_id)+" nbChilden="+to_string(nbChildren));
    // }
    // debugChildren();
    //EXIT();
    return 0;
}

int Feuille::brewsRealisable(Recipes brews, int& nbChildren, list<Chemin>& paths){
    for(int k=0; k<brews.size(); k++){
        Recipe brew = brews[k];
        if(isRealisable(brew)){
            Chemin idSpelsF;
            // DBG("----------- BREW realisable");  
            Feuille stateBrew(brew.m_actionId, *this+brew, eBREW, this);
            m_children.push_back(stateBrew);
            nbChildren++;          
            createListSpelsToBrew(stateBrew, idSpelsF);
            idSpelsF.setMembers(brew.m_price);
            paths.push_back(idSpelsF);
            // idSpelsF.debugChemin();
            return 0;
        }                        
    }
    return -1;
}

int Feuille::createListSpelsToBrew(Feuille& state, Chemin& idSpels){
    Feuille f = state;
    int id = state.m_id;
    while(id>0){
        idSpels.push_back(f);
        id = f.m_parent->m_id;
        f = *(f.m_parent);
    }
    idSpels.reverse();
    return idSpels.size();
}

// *****************************************************************************************
// ----------------- MAIN FUNCTION
// Auto-generated code below aims at helping you parse
// the standard input according to the problem statement.
// *****************************************************************************************
int main()
{
    bool getNewGoal = true;
    Chemin path;
    int nbTour = 0;

    // game loop
    while (1) {
        nbTour++;

        Values deltas, my_inventori, opp_inventori;
        Recipes recipes;

        int actionCount; // the number of spells and recipes in play
        cin >> actionCount; cin.ignore();
        for (int i = 0; i < actionCount; i++) {
            int actionId; // the unique ID of this spell or recipe
            string actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
            int delta0; // tier-0 ingredient change
            int delta1; // tier-1 ingredient change
            int delta2; // tier-2 ingredient change
            int delta3; // tier-3 ingredient change
            int price; // the price in rupees if this is a potion

            bool castable; // in the first league: always 0; later: 1 if this is a castable player spell

            int tomeIndex; // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax
            int taxCount; // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell
            bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
            cin >> actionId >> actionType >> delta0 >> delta1 >> delta2 >> delta3 >> price >> tomeIndex >> taxCount >> castable >> repeatable; cin.ignore();

            Recipe recipe(delta0, delta1, delta2, delta3, actionId, &actionType, price, castable);
            recipe.setLeague3(tomeIndex,taxCount, repeatable);
            recipes.push_back(recipe);
        }
        for (int i = 0; i < 2; i++) {
            Values inventori;
            int inv0; // tier-0 ingredients in inventory
            int inv1;
            int inv2;
            int inv3;
            int score; // amount of rupees
            cin >> inv0 >> inv1 >> inv2 >> inv3 >> score; cin.ignore();
            inventori.push_back(inv0);
            inventori.push_back(inv1);
            inventori.push_back(inv2);
            inventori.push_back(inv3);
            // inventori.m_score = score;

            if(i==0){
                my_inventori = inventori;
            }else{
                opp_inventori = inventori;
            }
            
        }

        Recipes brews, mySpells, oppSpells, grimSpels;
        recipes.getMySpells(mySpells);
        recipes.getOppSpells(oppSpells);
        recipes.getBrews(brews);
        recipes.getBookSpells(grimSpels);
        // grimSpels.debug();

        Strategy strategy(my_inventori, mySpells, brews, grimSpels);
        // if(nbTour>35){
        //    strategy.debugStrategy();
        // }
        stringstream out1;
        stringstream out;

        if(nbTour<TURNLEARN){
            out = strategy.strategyApproche3();
        }else{
            bool done = false;
            if(path.size()>0){
                // est ce que le goal existe encore?
                if(!strategy.m_brews.isInRecipes(path.back().m_id)){
                    DBG("----- GOAL DO NOT EXIST - "+to_string(path.back().m_id));
                    getNewGoal = true;
                }            
            }

            out = strategy.strategyApproche2(done, getNewGoal, path);
            if(!done){
                out1 = strategy.strategyApproche1(done);
                out << out1.str(); 
            }           
        }
        
        std::cout << out.str() << endl;
    }
}