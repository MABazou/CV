#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <utility>
#include <sstream>

using namespace std;

const double PI = acos(-1);

const int MAX_X = 17630;
const int MAX_Y = 9000;
const int MAX_MOVE = 800;
const int FOG_VIEW_UNIT = 2200;
const int FOG_VIEW_BASE = 6000;
const int MONSTER_SPEED = 400;

const int WIND_REACH = 1280;
const int WIND_MOOVE = 2200;
const int SHIELD_REACH = 2200;
const int CONTROL_REACH = 2200;


// seed=7192429763142570000
// T116 : Control pb x,y
// ***************************************************************************
// see CodersStrikeBack.cpp
// TODO:
//  + Vérifier la mana pour lancer un sort
//  + Vérifier iscontrol d'un monstre avant de lancer le sort control
//  - ne pas s'éloigner trop de ma base (ex: seed=-743819151885526140 T33)
//  - spell appellé plusieurs fois avec mes différents heros
//  - mettre en place differents rôles pour mes heros: 
//                  + défenseur : reste proche de la base
//                  + éclaireur/attaquant : part découvrir du terrain et tente de faire perdre l'ennemi 
//  - repousser les heros opp soit de ma base soit de la leur
//  - en strategy offensive, pousser les monstre vers la base
// ***************************************************************************


#define DEBUG
#ifdef DEBUG
#   define ENTER() do {cerr <<"D:ENTER - "<<__FUNCTION__ << endl; } while(0)
#   define EXIT() do {cerr << "D:EXIT  - "<<__FUNCTION__ << endl; } while(0)
#   define DBG(x) do {cerr <<"D:"<<__FUNCTION__<<": " << x << endl; } while(0)
#   define DBG_E(x) do {cerr <<"D:"<<__FUNCTION__<<": " << #x<<"="<<x << endl; } while(0)
#else 
#   define ENTER()
#   define EXIT()
#   define DBG(x) 
#   define DBG_E(x) 
#endif


/*! This is an enum class */
enum eSortType{
    eSortByDistance,
    eSortByScore
};
enum eUnitType {
    eMonster,         // 0 - Monter type enum
    eMyHero,          // 1 - My hero type enum
    eOppHero,          // 2 - Opponent hero type enum
    ePoint
};
enum eDirection {
    eUpLeft,         // 0 - 
    eUpRight,        // 1 - 
    eDownRight,      // 2 - 
    eDownLeft        // 3 -
};
enum eActionType {
    eWait,
    eMove,
    eWind,
    eShield,
    eControl
};
enum eStrategyType{
    eStrategyNeutre,
    eStrategyAttack,
    eStrategyDefense
};

char convertActionType2Char(eActionType actionType){
    if(actionType==eMove){
        return 'M';
    }else if(actionType==eWind){
        return 'W';
    }else if(actionType==eShield){
        return 'S';
    }else if(actionType==eControl){
        return 'C';
    }else{
        return 'X';
    }
}


/// Class Point2D description.
/** Detailed description. */
class Point2D{
    public:
        int m_x;
        int m_y;
    
    Point2D(){}

    Point2D(int x, int y){
        m_x = x;
        m_y = y;
    }

    Point2D(const Point2D& p){
        m_x = p.m_x;
        m_y = p.m_y;
    }
    void debug(){
        string s = "("+to_string(m_x)+", "+to_string(m_y)+")";
        DBG(s);
    }

    void setPoint2D(const int x0, const int y0){
        m_x = x0;
        m_y = y0;
    }
    void setPoint2D(const Point2D pos){
        m_x = pos.m_x;
        m_y = pos.m_y;
    } 
    void getPoint2D( Point2D& coord){
        coord.m_x = m_x;
        coord.m_y = m_y;
    }

    float distance2(const Point2D& pt){
        float d = (pt.m_x-m_x)*(pt.m_x-m_x) + (pt.m_y-m_y)*(pt.m_y-m_y);
        return d;
    }      
    float distance2(int x, int y){
        float d = (x-m_x)*(x-m_x) + (y-m_y)*(y-m_y);
        return d;
    }  
    float distance2(){
        float d = m_x*m_x + m_y*m_y;
        return d;
    }
            
    float distance(const Point2D& pt){
        float d = sqrt(distance2(pt));
        return d;
    }       
    float distance(int x, int y){
        float d = sqrt(distance2(x,y));
        return d;
    }    
    float distance(){
        float d = sqrt(distance2());
        return d;
    }    
    
    int isClose(float& d){  
        d = distance();
        return ((d< MAX_MOVE) ? 1: 0);
    }

    eDirection cadran(){
        // dans le sens horaire en partant du haut gauche
        // 0 - haut gauche      1 - haut droit
        // 3 - bas gauche       2 - bas droit
        eDirection res = eUpLeft;
        if(m_x<0){
            if(m_y<0){
                res = eUpLeft;
            }else{
                res = eDownLeft;
            }
        }else{
            if(m_y<0){
                res = eUpRight;
            }else{
                res = eDownRight ;
            }
        }
        return res;
    }

    void nextPosition(const Point2D vistesse){
        // TODO check if in arena
        Point2D pNext;
        pNext.m_x = m_x + vistesse.m_x;
        pNext.m_y = m_y + vistesse.m_y;
        pNext.isInArena();
        pNext.debug();
        // m_x += vistesse.m_x;
        // m_y += vistesse.m_y;
        // isInArena();
    }

    bool isInArena(){
        if(m_x<0){
            m_x = 0;
            DBG("out x<0");
        }
        if(m_x>MAX_X){
            m_x = MAX_X;
            DBG("out X>MAX");
        }
        if(m_y<0){
            m_y = 0;
            DBG("out y<0");
        }
        if(m_y>MAX_Y){
            m_y = MAX_Y;
            DBG("out y>MAX");
        }
    }

    Point2D closest(Point2D a, Point2D b){
        float da = b.m_y - a.m_y;
        float db = a.m_x - b.m_x;
        float c1 = da*a.m_x + db*a.m_y;
        float c2 = -db*m_x + da*m_y;
        float det = da*da + db*db;
        float cx = 0;
        float cy = 0;
    
        if (det != 0) {
            cx = (da*c1 - db*c2) / det;
            cy = (da*c2 + db*c1) / det;
        } else {
            // Le point est déjà sur la droite
            cx = m_x;
            cy = m_y;
        }
    
        return Point2D(cx, cy);
    }    

};

class Unit : public Point2D{

    public:
        int m_sortType;

        int m_Id;
        eUnitType m_Type;    
        int m_ShieldLife;
        int m_IsControlled;
        eActionType m_Action;
        int m_IsAGoal;
    Unit(){}

    Unit(Point2D p, int id) : Point2D(p){
        m_Id = id;
        m_Type = ePoint;
        m_ShieldLife = 0;
        m_IsControlled = 0;
        m_Action = eWait;
        m_IsAGoal = -1;
    }
    Unit(int Id, int Type, int x, int y, int ShieldLife, int IsControlled):Point2D(x,y){
        m_Id = Id;
        m_Type = (eUnitType) Type;    
        m_ShieldLife = ShieldLife;
        m_IsControlled = IsControlled;
        m_Action = eWait;
        m_IsAGoal = -1;
    }
    Unit(const Unit& u) : Point2D(u.m_x,u.m_y){
        m_Id = u.m_Id;
        m_Type = u.m_Type;
        m_ShieldLife = u.m_ShieldLife;
        m_IsControlled = u.m_IsControlled;
        m_Action = u.m_Action;
        m_IsAGoal = u.m_IsAGoal;
    }

    void debug(){
        string s = to_string(m_Type)+" "+to_string(m_Id) +"\t("+to_string(m_x)+", "+to_string(m_y)+") \t"+to_string(m_ShieldLife)+" | "+to_string(m_IsControlled);
        DBG(s);
    }    
    void debugParam(){
        string s = to_string(m_Id) +"\t"+to_string(m_Action)+"|"+to_string(m_ShieldLife)+"|"+to_string(m_IsControlled);
        DBG(s);
    }
    float distanceBewteenUnit(const Unit& unit){
        return distance(unit.m_x, unit.m_y);
    }  
    float distanceBewteenUnit(const Point2D posA){
        return distance(posA.m_x, posA.m_y);
    }     
    stringstream getOut(){
        stringstream out;    
        #ifdef DEBUG
            out << "MOVE "<< m_x << " " << m_y <<" "<<m_Id ;
        #else
            out << "MOVE "<< m_x << " " << m_y ;
        #endif
  
        return out;
    }

    int isClose(const Unit& u){
        int res = -1;
        int d = distance2(u);
        // DBG_E(d);
        if(d<=CONTROL_REACH*CONTROL_REACH){
            res = m_Id;
        }
        return res;
    }
};

class Monster : public Unit{
    public:
        int m_Health;
        Point2D m_vitesse;
        int m_NearBase;
        int m_ThreatFor;

        eDirection m_vitesseDir;    
    
    Monster(){}

    Monster(Point2D p, int id) : Unit(id, ePoint, p.m_x, p.m_y, 0, 0){
        m_Health = 0;
        m_vitesse = Point2D(0,0);
        m_NearBase = 0;
        m_ThreatFor = 0;
        setVitesseDir();
    }

    Monster(int Id, int Type,int x, int y, int ShieldLife, int IsControlled,
        int Health, Point2D vitesse, int NearBase, int ThreatFor) : Unit(Id, Type, x, y, ShieldLife, IsControlled){
        m_Health = Health;
        m_vitesse = vitesse;
        m_NearBase = NearBase;
        m_ThreatFor = ThreatFor;
        setVitesseDir();
    }
    Monster(const Unit& u, int Health, Point2D vitesse, int NearBase, int ThreatFor) : Unit(u){
        m_Health = Health;
        m_vitesse = vitesse;
        m_NearBase = NearBase;
        m_ThreatFor = ThreatFor;
        setVitesseDir();
    }
    Monster(const Unit& u) : Unit(u){
        m_Health = 0;
        m_vitesse = Point2D(0,0);
        m_NearBase = 0;
        m_ThreatFor = 0;
        setVitesseDir();
    }    
    Monster(const Monster& m):Unit(m){
        m_Health = m.m_Health;
        m_vitesse = m.m_vitesse;
        m_NearBase = m.m_NearBase;
        m_ThreatFor = m.m_ThreatFor;
        setVitesseDir();
    }

    void debug(){
        string s = to_string(m_Type)+" "+to_string(m_Id) +"\t("+to_string(m_x)+", "+to_string(m_y)+") \t vDir= "+to_string(m_vitesseDir)+" | "+to_string(m_ShieldLife);
        s += "\t H,N,T :"+to_string(m_Health)+", "+to_string(m_NearBase)+", "+to_string(m_ThreatFor);
        DBG(s);
    }  

    void setVitesseDir(){
        m_vitesseDir = m_vitesse.cadran();
    }
    float distanceBewteenUnit(const Monster& unit){
        return distance(unit.m_x, unit.m_y);
    }  
    float distanceBewteenUnit(const Point2D posA){
        return distance(posA.m_x, posA.m_y);
    }  
    
    // Update position values with speed and time
    void move(float t){
        m_x += m_vitesse.m_x * t;
        m_y += m_vitesse.m_y * t;
    }    

    eActionType monsterIsDaugerous(int distToMyBase){
        // si danger (ie H/2 > distance_to_my_base/step) -> SPELL
        eActionType spell = eMove;

        int nbTurnsToKillMonster = (m_Health+1)/2 ;
        // string s = "WARNING - "+to_string(m_Id)+" "+to_string(nbTurnsToKillMonster)+">="+to_string(distToMyBase/MONSTER_SPEED);
        // DBG(s);

        if(nbTurnsToKillMonster >= (distToMyBase+WIND_MOOVE)/MONSTER_SPEED){
            spell = eControl;
        }else{
            if(nbTurnsToKillMonster >= distToMyBase/MONSTER_SPEED){ 
                spell = eWind;
            }  
        } 
        return spell;
    }

};

class Monsters : public vector<Monster>{
    public:
        int m_sortType;
    
    void debugAll(){
        ENTER();
        if(size()>0){
            string s = "Type Id (x,y) vDir|Shield --- "+to_string(size());
            DBG(s);
        }
        for (int i = 0; i < size(); i++) {
            at(i).debug();
        }
        EXIT();
    }
    void debugAllParam(){
        ENTER();
        DBG("Type   Id \t Action | ShieldLife | IsControlled");
        for (int i = 0; i < size(); i++) {
            at(i).debugParam();
        }
        EXIT();
    }

    bool operator() (const Monster& unitA, const Monster& unitB) { 
        float dA, dB;
        switch(m_sortType){
            case eSortByDistance:
                dA = centeredUnit.distanceBewteenUnit(unitA);
                dB = centeredUnit.distanceBewteenUnit(unitB);
                return (dA<dB);
                break;
            default:
                break;
            
        }
    }

    void sortByDistanceFromOrigin () {
        centeredUnit.m_x = 0;
        centeredUnit.m_y = 0;
        m_sortType=eSortByDistance;
        std::sort (begin(), end(), *this); 
    }    
    void sortByDistanceFromPoint(const Point2D& pos) {
        centeredUnit.m_x = pos.m_x;
        centeredUnit.m_y = pos.m_y;
        m_sortType=eSortByDistance;
        std::sort (begin(), end(), *this); 
    }
    void sortByDistance(const Monster& unitA) {
        centeredUnit = unitA;
        m_sortType=eSortByDistance;
        std::sort (begin(), end(), *this); 
    }   
    int filterByVdir(eDirection dir, Monsters& units){
        for(int i= 0; i<size(); i++){
            if(at(i).m_vitesseDir==dir){
                units.push_back(at(i));
            }
        }
        return units.size();
    }    

    int filterById(int id){
        for(int i= 0; i<size(); i++){
            if(at(i).m_Id==id){
                return i;
            }
        }
        return -1;
    }          

    int isDangerous(Point2D& base){
        for(int i= 0; i<size(); i++){
            int dist = at(i).distance(base);
            if(dist < FOG_VIEW_BASE){
                return i;
            }     
        }
        return -1;
    }

    stringstream getOut(int i){
        stringstream out;
        Unit target;
    
        if (size() > 0){
            target = at(i);
            #ifdef DEBUG
                out << "MOVE "<< target.m_x << " " << target.m_y <<" "<<target.m_Id ;
            #else
                out << "MOVE "<< target.m_x << " " << target.m_y ;
            #endif
        } else{
            out << "WAIT" ;
        }    
        return out;
    }

    private:
        Monster centeredUnit;

};

class Players : public vector<Unit>{
    public:
        Point2D m_Base;
        int m_Health;
        int m_Mana;
        int m_sortType;

    void setMyBase(int x, int y){
        m_Base.m_x = x;
        m_Base.m_y = y;
    }    
    void setMyBase(Point2D p){
        m_Base.setPoint2D(p);
    }    
    void setOppBase(Point2D p){
        m_Base.setPoint2D(MAX_X-p.m_x, MAX_Y-p.m_y);
    }

    void setParam(int health, int mana){
        m_Health = health;
        m_Mana = mana;
    }


    void debugAll(){
        ENTER();
        DBG("Type   Id  (x,y)   ShieldLife      IsControlled");

        for (int i = 0; i < size(); i++) {
            at(i).debug();
        }
        EXIT();
    }    
    void debugAllParam(){
        ENTER();
        DBG("Type   Id \t Action | ShieldLife | IsControlled");
        for (int i = 0; i < size(); i++) {
            at(i).debugParam();
        }
        EXIT();
    }

    bool operator() (const Unit& unitA, const Unit& unitB) { 
        float dA, dB;
        switch(m_sortType){
            case eSortByDistance:
                dA = centeredUnit.distanceBewteenUnit(unitA);
                dB = centeredUnit.distanceBewteenUnit(unitB);
                return (dA<dB);
                break;
            default:
                break;
            
        }
    }
    void sortByDistanceFromPoint(const Point2D& pos) {
        centeredUnit.m_x = pos.m_x;
        centeredUnit.m_y = pos.m_y;
        m_sortType=eSortByDistance;
        std::sort (begin(), end(), *this); 
    }

    int filterById(int id){
        for(int i= 0; i<size(); i++){
            if(at(i).m_Id==id){
                return i;
            }
        }
        return -1;
    }  
    
    Point2D getInitPos(int i=1, eStrategyType strat=eStrategyNeutre){
        Point2D target;
        // if(strat == eStrategyNeutre){
            // on the circle x² + y² = r²
            // PI/6 -> PI/8
            if(m_Base.m_x == 0){
                float x = FOG_VIEW_BASE*cos((i+1)*PI/6-PI/12);
                float y = FOG_VIEW_BASE*sin((i+1)*PI/6-PI/12);
                target.setPoint2D(x,y);                
            }else{
                float x = m_Base.m_x-FOG_VIEW_BASE*cos(i*PI/6+PI/12);
                float y = m_Base.m_y-FOG_VIEW_BASE*sin(i*PI/6+PI/12);
                target.setPoint2D(x,y);                    
            }

        // }else{
        //     target.setPoint2D(MAX_X/2,MAX_Y/2);
        // }
        return target;
    }
    Point2D getInitAttackPos(int i=1, eStrategyType strat=eStrategyNeutre){
        Point2D target;
        // if(strat == eStrategyNeutre){
            // on the circle x² + y² = r²
            // PI/6 -> PI/8
            float coef = 2./3.;
            if(m_Base.m_x == 0){
                float x = coef*FOG_VIEW_BASE*cos((i+1)*PI/6-PI/12);
                float y = coef*FOG_VIEW_BASE*sin((i+1)*PI/6-PI/12);
                target.setPoint2D(x,y);                
            }else{
                float x = m_Base.m_x-coef*FOG_VIEW_BASE*cos(i*PI/6+PI/12);
                float y = m_Base.m_y-coef*FOG_VIEW_BASE*sin(i*PI/6+PI/12);
                target.setPoint2D(x,y);                    
            }

        // }else{
        //     target.setPoint2D(MAX_X/2,MAX_Y/2);
        // }
        return target;
    }    
  
    int isOppCloseToMe(Unit& myUnit){
        // apply on opp players
        int res = -1;
        for(int i=0; i<size(); i++){            
            if(at(i).isClose(myUnit) != -1){
                res = i;
                break;
            }                       
        }
        return res;
    }

    void strategy(Unit* targetUnits, int i, Monsters& myMonsters, Monsters& neutralMonsters, Players& oppPlayers){
        // warning use i-1
        // TODO - ne pas s'éloigner trop de ma base
        at(i).m_Action = eWait;
        string s = "---- DEBUG ----  "+to_string(i)+"  ";

        if(myMonsters.size()==0 && neutralMonsters.size()==0){
            at(i).m_Action = eMove;
            s += "MOVE init 000 ";
            // if(i==0){
            //     Point2D res = oppPlayers.getInitPos(i);
            //     targetUnits[i].setPoint2D(res);             
            // }else{
                Point2D res = getInitPos(i);
                targetUnits[i].setPoint2D(res);                
            // }

            targetUnits[i].m_Id = at(i).m_Id;
        }else{
            int isOppDangerous = oppPlayers.isOppCloseToMe(at(i));  // action sur opp player (shield ou control)
            int isMonsterDangerous = myMonsters.isDangerous(m_Base);
            if(m_Mana>=30 && ( isOppDangerous!=-1 || isMonsterDangerous!=-1 )){
                s += "SPELL? ";
                // on opp player then on monster TODO - adapt and modify
                if(isOppDangerous!=-1){
                    Unit opp = oppPlayers.at(isOppDangerous);
                    int distHeroOpp = at(i).distance(opp);
                    if(distHeroOpp<CONTROL_REACH && opp.m_ShieldLife==0 && opp.m_IsControlled==0){
                        // --- APPLY CONTROL on opponent
                        s += "APPLY CONTROL on opponent";
                        at(i).m_Action = eControl;
                        targetUnits[i].m_Id = opp.m_Id;
                        targetUnits[i].m_x = oppPlayers.m_Base.m_x;
                        targetUnits[i].m_y = m_Base.m_y;
                        oppPlayers.at(isOppDangerous).m_IsControlled = 1;
                    }else if(at(i).m_ShieldLife==0){
                        // --- APPLY SHIELD on my entity
                        s += "APPLY SHIELD on my entity";
                        at(i).m_Action = eShield;
                        targetUnits[i].m_Id = at(i).m_Id;
                        targetUnits[i].m_x = i;
                        targetUnits[i].m_y = i; 
                        at(i).m_ShieldLife = 1;              
                    }
                }else if(isMonsterDangerous != -1){
                    Monster m = myMonsters.at((i-1)% myMonsters.size());
                    int dist = m_Base.distance(m);
                    int distHeroMonter = at(i).distance(m);
                    eActionType act = m.monsterIsDaugerous(dist); // action sur monstre (control ou wind)

                    if(act==eWind && distHeroMonter<WIND_REACH && m.m_IsControlled==0){
                        // --- APPLY WIND on monster si hero proche du danger
                        at(i).m_Action = eWind;
                        s += "APPLY WIND on monster";
                        targetUnits[i].m_Id = m.m_Id;
                        targetUnits[i].m_x = oppPlayers.m_Base.m_x;
                        targetUnits[i].m_y = oppPlayers.m_Base.m_y;

                    }else if(act==eControl && distHeroMonter<CONTROL_REACH && m.m_IsControlled==0){
                        // --- APPLY CONTROL on monster si hero proche du danger
                        at(i).m_Action = eControl;
                        s += "APPLY CONTROL on monster";
                        targetUnits[i].m_Id = m.m_Id;
                        targetUnits[i].m_x = oppPlayers.m_Base.m_x;
                        targetUnits[i].m_y = oppPlayers.m_Base.m_y;
                        myMonsters.at((i-1)% myMonsters.size()).m_IsControlled = 1;

                    }
                }
            }
        }

        if(at(i).m_Action == eWait){
            // --- APPLY MOVE and Keep Target
            if(myMonsters.size()>0){
                at(i).m_Action = eMove;
                s += "MOVE target myMonsters";
                int stillExist = myMonsters.filterById(targetUnits[i].m_Id);
                if(stillExist == -1){
                    targetUnits[i] = myMonsters.at((i-1)% myMonsters.size());
                    // targetUnits[i] = myMonsters.at(0);
                }else{
                    targetUnits[i] = myMonsters.at(stillExist);
                }
            }else if(neutralMonsters.size()>0){
                at(i).m_Action = eMove;
                s += "MOVE target neutralMonsters";
                neutralMonsters.sortByDistanceFromPoint(at(i));
                targetUnits[i] = neutralMonsters.at((i-1) % neutralMonsters.size());
            }
        }

        s += " "+to_string(targetUnits[i].m_x)+", "+to_string(targetUnits[i].m_y);
        s += " -- "+to_string(at(i).m_Action);
        // DBG(s);
    }

    void strategyOffensive(Unit* targetUnits, int i, Monsters& oppMonsters, Monsters& neutralMonsters, Players& oppPlayers){
        string s = "---- DEBUG ----  "+to_string(i)+"  ";
        at(i).m_Action = eWait;
        // s += "MOVE init 000";   
        if(oppMonsters.size()>0){
            Monster m0 = oppMonsters.at(0);

            int distHeroM0 = at(i).distance(m0);
            int distHeroOppBase = at(i).distance(oppPlayers.m_Base);


            if(m_Mana>=30 && distHeroM0<WIND_REACH && distHeroOppBase<FOG_VIEW_BASE*2 && m0.m_IsControlled==0){
                // --- APPLY WIND on monster si hero proche du danger
                at(i).m_Action = eWind;
                s += "APPLY WIND on monster";
                targetUnits[i].m_Id = m0.m_Id;
                targetUnits[i].m_x = oppPlayers.m_Base.m_x;
                targetUnits[i].m_y = oppPlayers.m_Base.m_y;

            }

            // if(m_Mana>=30 && distHeroM0<CONTROL_REACH && distHeroOppBase<FOG_VIEW_BASE*2 && m0.m_IsControlled==0){
            //     at(i).m_Action = eControl;
            //     // s += "APPLY CONTROL on monster";
            //     targetUnits[i].m_Id = m0.m_Id;
            //     targetUnits[i].m_x = oppPlayers.m_Base.m_x;
            //     targetUnits[i].m_y = oppPlayers.m_Base.m_y;                      
            // }
        }else if(neutralMonsters.size()>0){
            neutralMonsters.sortByDistanceFromPoint(at(i));
            Monster m = neutralMonsters.at(0);

            if (m_Mana<100){
                at(i).m_Action = eMove;
                targetUnits[i].setPoint2D(m); 
                targetUnits[i].m_Id = m.m_Id;                  
            }else{
                // at(i).m_Action = eWind;
                // // s += "APPLY WIND on monster";
                int distHeroMonter = at(i).distance(m);
                if(distHeroMonter<CONTROL_REACH && m.m_IsControlled==0){
                    at(i).m_Action = eControl;
                    // s += "APPLY CONTROL on monster";
                    targetUnits[i].m_Id = m.m_Id;
                    targetUnits[i].m_x = oppPlayers.m_Base.m_x;
                    targetUnits[i].m_y = oppPlayers.m_Base.m_y;                      
                }
                          
            }
      
        }
        if(at(i).m_Action ==eWait){
            at(i).m_Action = eMove;
            Point2D res = oppPlayers.getInitAttackPos(i);
            targetUnits[i].setPoint2D(res); 
            targetUnits[i].m_Id = at(i).m_Id;                 
        }
        s += to_string(targetUnits[i].m_x)+", "+to_string(targetUnits[i].m_y);
        // DBG(s);      
   
    }

    stringstream getOut(Unit* targetUnits, int i){
        stringstream out;
        if(at(i).m_Action == eMove){
            #ifdef DEBUG
                out << "MOVE "<< targetUnits[i].m_x << " " << targetUnits[i].m_y <<" H"<<i<<"-"<< targetUnits[i].m_Id ;
            #else
                out << "MOVE "<< targetUnits[i].m_x << " " << targetUnits[i].m_y ;
            #endif 
        }else if(at(i).m_Action == eWind){
            #ifdef DEBUG
                out << "SPELL WIND "<< targetUnits[i].m_x << " " << targetUnits[i].m_y << " W" <<i ;
            #else
                out << "SPELL WIND "<< targetUnits[i].m_x << " " << targetUnits[i].m_y ;
            #endif 
        }else if(at(i).m_Action == eShield){
            #ifdef DEBUG
                out << "SPELL SHIELD "<< targetUnits[i].m_Id <<" S"<<i ;
            #else
                out << "SPELL SHIELD "<< targetUnits[i].m_Id ;
            #endif 
        }else if(at(i).m_Action == eControl){
            #ifdef DEBUG
                out << "SPELL CONTROL "<< targetUnits[i].m_Id << " " << targetUnits[i].m_x << " " << targetUnits[i].m_y <<" C"<<i ;
            #else
                out << "SPELL CONTROL "<< targetUnits[i].m_Id << " " << targetUnits[i].m_x << " " << targetUnits[i].m_y ;
            #endif 
        }else{
            out << "WAIT";
        }


        return out;
    }
    private:
        Unit centeredUnit;
};

class Game{
    public:
        Players m_myHeros;
        Players m_oppHeros;
        Monsters m_myMonsters;
        Monsters m_oppMonsters;
        Monsters m_neutralMonsters;
    
    Game(){}
    Game(const Players& myHeros, const Players& oppHeros, const Monsters& myMonsters, const Monsters& oppMonsters, const Monsters& neutralMonsters){
        // ENTER();
        m_myHeros = myHeros;
        m_oppHeros = oppHeros;
        m_myMonsters = myMonsters;
        m_oppMonsters = oppMonsters;
        m_neutralMonsters = neutralMonsters;
        // EXIT();
    }
};

class ScoreAction{
    public:    
        Unit m_Hero;
        Point2D m_Base;
        eActionType m_Action;
        Monster m_Target;
        int m_IdTarget;
        float m_Score;

    ScoreAction(){}

    ScoreAction(const Unit& hero, eActionType action, const Monster& target){
        // ENTER();
        m_Hero = hero;
        m_Action = action;
        m_Target = target;
        m_IdTarget = target.m_Id;
        m_Score = 0;
        // EXIT();
    }
    ScoreAction(const Unit& hero, eActionType action, const Unit& target){
        // ENTER();
        m_Hero = hero;
        m_Action = action;
        m_Target = target;
        m_IdTarget = target.m_Id;
        m_Score = 0;
        // EXIT();
    }

    void debug(){
        string res = "idH, idT, score : "+to_string(m_Hero.m_Id)+", "+to_string(m_IdTarget)+", "+to_string(m_Score);
        DBG(res);
    }

    void calculateScore(const Game& game){
        // maximiser le score et prendre le target max
        //m_Hero.debug();

        float distHeroToTarget = m_Hero.distance(m_Target);

        float distTagetToMyBase = m_Target.distance(game.m_myHeros.m_Base)+1;
        // distTagetToMyBase -= FOG_VIEW_BASE;

        float distHeroToMyBase = m_Hero.distance(game.m_myHeros.m_Base);

        float distHeroToOppBase = m_Hero.distance(game.m_oppHeros.m_Base);
        
        Point2D ptMAX(MAX_X,MAX_Y);
        float distHeroToOppPlayer = ptMAX.distance();        
        if(game.m_oppHeros.size()>0){
            Players p = game.m_oppHeros;
            p.sortByDistanceFromPoint(m_Hero);
            distHeroToOppPlayer = p.at(0).distance(m_Hero);
        }
        float coefDistance = 50;

        // DBG_E(m_Target.m_Type);
        // DBG_E(distHeroToTarget);
        // DBG_E(distTagetToMyBase);

        switch (m_Action)
        {
            case eWait:
                m_Score += 10;
                break;
            case eMove:
                m_Score += 100;
                if(m_Target.m_IsAGoal == -1){
                    if(game.m_myHeros.m_Mana<10) m_Score += 20;
                    if(m_Target.m_Type == ePoint){
                        if(distTagetToMyBase < FOG_VIEW_BASE ) m_Score -= 50;
                        if(distHeroToOppBase < FOG_VIEW_BASE ) m_Score += 10;
                        m_Score += 1/(1+distHeroToTarget);
                    } 
                    if(m_Target.m_Type == eMonster){
                        m_Score += 50;
                        if(m_Target.m_NearBase == 1 && m_Target.m_ThreatFor==1) m_Score += 50;

                        int val = (FOG_VIEW_BASE - distTagetToMyBase)/coefDistance;
                        if(distTagetToMyBase < FOG_VIEW_BASE ) m_Score += 50+val;
                        if(distHeroToOppBase < FOG_VIEW_BASE ) m_Score += 10;
                    } 
                    if(m_Target.m_Type == eOppHero) m_Score = 0;
                }
                //m_Score -= distHeroToTarget/coefDistance;
                //m_Score -= abs(distTagetToMyBase)/(coefDistance*2);

                break;
            case eWind:
                m_Score += 80;

                if(game.m_myHeros.m_Mana>10 && distTagetToMyBase < FOG_VIEW_BASE && distHeroToTarget<WIND_REACH) m_Score += 150;
                if(distHeroToTarget<WIND_REACH) m_Score += 20;
                if(distHeroToMyBase<FOG_VIEW_BASE){
                    m_Score -= (distHeroToMyBase+distHeroToTarget)/coefDistance;
                }
                if(distHeroToOppBase<FOG_VIEW_BASE){
                    m_Score -= (distHeroToOppBase+distHeroToTarget)/coefDistance;
                }
                if(game.m_myHeros.m_Mana<10) m_Score = -20;
                break;
            case eShield:
                if(m_Target.m_Type == eOppHero) m_Score += 20;
                if(distHeroToTarget<=SHIELD_REACH) m_Score += 50;
                if(distTagetToMyBase<FOG_VIEW_BASE*3/2) m_Score += 50;
                // m_Score -= distHeroToOppPlayer/coefDistance;
                if(game.m_myHeros.m_Mana<10) m_Score = -20;
                break;
            case eControl:
                m_Score += 1;
                if(distHeroToTarget<=CONTROL_REACH) m_Score += 50;
                if(game.m_myHeros.m_Mana<10) m_Score = -20;
                break;                                    
            
            default:
                break;
        }

        // string s = "\t"+to_string(m_IdTarget)+",\t"+to_string(m_Score);
        // DBG(s);

    }

    stringstream getOut(Game& game, int i){
        stringstream out;
        if(m_Action == eMove){
            #ifdef DEBUG
                out << "MOVE "<< m_Target.m_x << " " << m_Target.m_y <<" H"<<i<<"-"<< m_Target.m_Id ;
            #else
                out << "MOVE "<< m_Target.m_x << " " << m_Target.m_y ;
            #endif 
        }else if(m_Action == eWind){
            // #ifdef DEBUG
            //     out << "SPELL WIND "<< m_Target.m_x << " " << m_Target.m_y << " W" <<i ;
            // #else
            //     out << "SPELL WIND "<< m_Target.m_x << " " << m_Target.m_y  ;
            // #endif 
            #ifdef DEBUG
                out << "SPELL WIND "<< game.m_oppHeros.m_Base.m_x << " " << game.m_oppHeros.m_Base.m_y << " W" <<i ;
            #else
                out << "SPELL WIND "<< game.m_oppHeros.m_Base.m_x << " " << game.m_oppHeros.m_Base.m_y  ;
            #endif             
        }else if(m_Action == eShield){
            #ifdef DEBUG
                out << "SPELL SHIELD "<< m_Target.m_Id <<" S"<<i ;
            #else
                out << "SPELL SHIELD "<< m_Target.m_Id ;
            #endif 
        }else if(m_Action == eControl){
            #ifdef DEBUG
                out << "SPELL CONTROL "<< m_Target.m_Id << " " << m_Target.m_x << " " << m_Target.m_y  <<" C"<<i ;
            #else
                out << "SPELL CONTROL "<< m_Target.m_Id << " " << m_Target.m_x << " " << m_Target.m_y  ;
            #endif 
        }else{
            out << "WAIT";
        }


        return out;
    }

};

class ScoreActions : public vector<ScoreAction>{
    public:
        Game m_Game;
        eSortType m_sortType;
        bool m_IsSortCroissant;

    ScoreActions(){}
    ScoreActions(const Game& game){
        m_Game = game;
    }

    void debugAll(){
        for(int i=0; i<size(); i++){
            at(i).debug();
        }
    }

    // void setGame(const Game& game){
    //     m_Game = game;
    // }
    bool operator() (const ScoreAction& unitA, const ScoreAction& unitB) { 
        float dA, dB;
        switch(m_sortType){
            case eSortByScore:
                dA = unitA.m_Score;
                dB = unitB.m_Score;
                return m_IsSortCroissant?(dA<dB):(dA>dB);
                break;
            default:
                break;
            
        }
    }

    void sortByScore(bool rev=true) {
        m_sortType=eSortByScore;
        m_IsSortCroissant = rev;
        std::sort (begin(), end(), *this); 
    }


    /*
    9 points par défaut: 
    - origine, end map, middle
    - 3 points outside of my base
    - 3 points inside of the opp base (currently also outside)
    */
    void fixInterestedPoints(Point2D p[9]){
        // ENTER();
        // Point2D p[9];
        // DBG("ok");

        p[0] = Point2D(0,0);                // origin
        p[1] = Point2D(MAX_X,MAX_Y);        // end map
        p[2] = Point2D(MAX_X/2,MAX_Y/2);    // middle

        // Outside base (0,0)
        p[3] = Point2D(5795, 1552);    
        p[4] = Point2D(4242, 4242);
        p[5] = Point2D(1552, 5795);

        // Outside base (MAX_X,MAX_Y)
        p[6] = Point2D(11834, 7447);    
        p[7] = Point2D(13387, 4757);
        p[8] = Point2D(16077, 3204);
        // EXIT();
    }

    void initScoreActions(const Unit& player, eActionType actType){
        // ENTER();
        //DBG_E(actType);

        // int indMyHero = 0;
        // Unit player = m_Game.m_myHeros[indMyHero];
        
        // DBG("-- TEST SCORING 1 action various points "); // 1 à 9

        if(actType==eMove && m_Game.m_myMonsters.size()==0 && m_Game.m_oppMonsters.size()==0 && m_Game.m_neutralMonsters.size()==0){
            Point2D pt[9];
            fixInterestedPoints(pt);
            //for(int i=0; i<9; i++){
            //for(int i=3; i<6; i++){
                //pt[i].debug();
                int i = 3+player.m_Id;
                Monster u(pt[i], -i-1);
                ScoreAction sca0(player, actType, u);
                sca0.calculateScore(m_Game);
                push_back(sca0);
            //}            
        }       


        for(int i=0; i<m_Game.m_myMonsters.size(); i++){
            ScoreAction sca(player, actType, m_Game.m_myMonsters[i]);
            sca.calculateScore(m_Game);
            push_back(sca);
        }
        for(int i=0; i<m_Game.m_neutralMonsters.size(); i++){
            ScoreAction sca(player, actType, m_Game.m_neutralMonsters[i]);
            sca.calculateScore(m_Game);
            push_back(sca);
        }
        for(int i=0; i<m_Game.m_oppMonsters.size(); i++){
            ScoreAction sca(player, actType, m_Game.m_oppMonsters[i]);
            sca.calculateScore(m_Game);
            push_back(sca);
        } 

        if(actType!=eMove){
            for(int i=0; i<m_Game.m_oppHeros.size(); i++){
                ScoreAction sca(player, actType, m_Game.m_oppHeros[i]);
                sca.calculateScore(m_Game);
                push_back(sca);
            }              
        } 
              

/*
        DBG("-- TEST SCORING 1 point various actions ");
        Point2D pt0 = pt[0];
        if(m_Game.m_myMonsters.size()>0){
            pt0 = Point2D(m_Game.m_myMonsters.at(0));
        }
        pt0.debug();

        ScoreAction sca0(m_Game.m_myHeros[indMyHero], eMove, pt0, 0);
        sca0.calculateScore(m_Game);
        push_back(sca0);

        ScoreAction sca1(m_Game.m_myHeros[indMyHero], eWind, pt0, 1);
        ScoreAction sca2(m_Game.m_myHeros[indMyHero], eShield, pt0, 2);
        ScoreAction sca3(m_Game.m_myHeros[indMyHero], eControl, pt0, 3);
        sca1.calculateScore(m_Game);
        sca2.calculateScore(m_Game);
        sca3.calculateScore(m_Game);   
        push_back(sca1);
        push_back(sca2);
        push_back(sca3);
*/

        // DBG_E(at(0).m_IdTarget);
        // DBG_E(size());
        // EXIT();
    }


    private: 
        // Unit centeredUnit;

};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    int base_x; // The corner of the map representing your base
    int base_y;
    cin >> base_x >> base_y; cin.ignore();
    int heroes_per_player; // Always 3
    cin >> heroes_per_player; cin.ignore();

    Point2D myBase(base_x,base_y);
    myBase.debug();
    Unit myHerosTarget[heroes_per_player];

    // game loop
    while (1) {
        Players myHeroes, oppHeroes;
        Monsters myMonsters, oppMonsters, neutralMonsters;
        myHeroes.setMyBase(myBase);
        oppHeroes.setOppBase(myBase);

        for (int i = 0; i < 2; i++) {
            int health; // Your base health
            int mana; // Ignore in the first league; Spend ten mana to cast a spell
            cin >> health >> mana; cin.ignore();
            if(i==0){
                myHeroes.setParam(health,mana);
            }else{
                oppHeroes.setParam(health,mana);
            }
        }
        int entity_count; // Amount of heros and monsters you can see
        cin >> entity_count; cin.ignore();

        for (int i = 0; i < entity_count; i++) {
            int id; // Unique identifier
            int type; // 0=monster, 1=your hero, 2=opponent hero
            int x; // Position of this entity
            int y;
            int shield_life; // Ignore for this league; Count down until shield spell fades
            int is_controlled; // Ignore for this league; Equals 1 when this entity is under a control spell
            
            int health; // Remaining health of this monster
            int vx; // Trajectory of this monster
            int vy;
            int near_base; // 0=monster with no target yet, 1=monster targeting a base
            int threat_for; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
            cin >> id >> type >> x >> y >> shield_life >> is_controlled >> health >> vx >> vy >> near_base >> threat_for; cin.ignore();
            Unit entity(id, type, x, y, shield_life, is_controlled);
            Monster monster(entity, health, Point2D(vx,vy), near_base, threat_for);

            if(type == eMyHero){
                myHeroes.push_back(entity);
            }
            if(type == eOppHero){
                oppHeroes.push_back(entity);
            }
            if(type == eMonster && threat_for == 0){
                neutralMonsters.push_back(monster);
            }
            if(type == eMonster && threat_for == 1){
                myMonsters.push_back(monster);
            }
            if(type == eMonster && threat_for == 2){
                oppMonsters.push_back(monster);
            }
        }

        myMonsters.sortByDistanceFromPoint(myBase);

        // myMonsters.debugAll();
        // neutralMonsters.debugAll();



        // myMonsters.debugAllParam();
        // myHeroes.debugAllParam();

        // DBG_E(myMonsters.size());
        // DBG_E(neutralMonsters.size());
        // DBG_E(oppHeroes.size());

        for (int i = 0; i < heroes_per_player; i++) {

            DBG("----------- DEBUG --------------");
            Game game(myHeroes,oppHeroes,myMonsters,oppMonsters,neutralMonsters);
            ScoreActions test(game);
            // int indMyHero = 0;
            Unit player = myHeroes[i];
            // player.debug();
            //if(game.m_myHeros.m_Mana<30 || i==0){
            test.initScoreActions(player, eMove);            
            test.initScoreActions(player, eWind);
            test.initScoreActions(player, eShield);
            test.initScoreActions(player, eControl);

            //test.debugAll();
            DBG("----------- DEBUG SORT --------------");
            test.sortByScore(false);
            //test.debugAll();

            int ind = myMonsters.filterById(test.at(0).m_IdTarget);
            if(player.m_IsControlled==0){
                if(ind != -1){
                    myMonsters.at(ind).m_IsAGoal = player.m_Id;
                }else{
                    ind = neutralMonsters.filterById(test.at(0).m_IdTarget);
                    if(ind != -1){
                        neutralMonsters.at(ind).m_IsAGoal = player.m_Id;
                    }else{
                        ind = oppMonsters.filterById(test.at(0).m_IdTarget);
                        if(ind != -1){
                            oppMonsters.at(ind).m_IsAGoal = player.m_Id;
                        }
                    }
                }
            }
            stringstream out = test.at(0).getOut(game, i);
            DBG_E(out.str() );     
            DBG("----------- DEBUG END--------------");




            // if(i==6){
            //     myHeroes.strategyOffensive(myHerosTarget, i, oppMonsters, neutralMonsters, oppHeroes);
            // }else{
            //     myHeroes.strategy(myHerosTarget, i, myMonsters, neutralMonsters, oppHeroes);
            // }
            // stringstream out = myHeroes.getOut(myHerosTarget, i);

            cout << out.str() << endl; 
            
        }
    }
}
