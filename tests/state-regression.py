"""Run production armor-state methods through a small C++ host adapter.

This checks state transitions and queued damage, not DayZ compilation or engine
callback ordering. Requires Python 3 and g++; generated files stay temporary.
"""
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('Scripts/4_World/ArPen/ArPenArmorState.c').read_text()
source = source.replace('modded class ItemBase', 'class ItemBase : public ItemRuntime')
source = source.replace('{\n    protected bool', '{\npublic:\n    protected bool', 1)
source = re.sub(r'\b(protected|override|ref)\s+', '', source)
source = source.replace('array<float>', 'Array<float>').replace('new Array<float>', 'Array<float>()')
source = source.replace('ArPenArmorData armorData', 'ArPenArmorData& armorData').replace('ArPenHitResult hit', 'ArPenHitResult& hit')
source = source.replace('super.', 'ItemRuntime::').replace('Math.', 'Math::').replace('ArPenConfig.', 'ArPenConfig::')
source = source.replace('foreach (float health : m_ArPenTileHealth)', 'for (float health : m_ArPenTileHealth)')
source = re.sub(r'\b(bool|float|int) (\w+);', r'\1 \2 = 0;', source)
source = source.replace('GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyItemDamage, 0, false, itemDamage);', 'queue.push_back([this, itemDamage]() { ArPen_ApplyItemDamage(itemDamage); });')
source = source.replace('GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ArPen_ApplyItemDamage);', 'queue.clear();')
prelude = r'''
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
template<class T> struct Array : std::vector<T> {
    int Count() const { return this->size(); }
    void Insert(T value) { this->push_back(value); }
    void Clear() { this->clear(); }
    explicit operator bool() const { return true; }
};
struct Math {
    static float Max(float a,float b){return std::max(a,b);}
    static float Min(float a,float b){return std::min(a,b);}
    static float Clamp(float x,float lo,float hi){return std::clamp(x,lo,hi);}
    static float AbsFloat(float x){return std::abs(x);}
};
struct Context {
    std::shared_ptr<std::vector<std::any>> values = std::make_shared<std::vector<std::any>>();
    std::shared_ptr<size_t> position = std::make_shared<size_t>(0);
    template<class T> void Write(T v) { values->push_back(v); }
    template<class T> bool Read(T& v) { if (*position >= values->size()) return false; v = std::any_cast<T>((*values)[(*position)++]); return true; }
};
using ParamsWriteContext = Context;
using ParamsReadContext = Context;
struct ArPenArmorData {
    float BaseKrupp=1500, BaseArmorHealth=800, SurfaceAreaCM2=2500, ThicknessMM=24;
    Array<float> Tiles;
};
struct ArPenConfig { static int TileCount(ArPenArmorData& a) { return a.Tiles.Count(); } };
struct ArPenHitResult { float ArmorDamage=0, AddedMetalLossVolumeMM3=0, AddedDentVolumeMM3=0; int TileIndex=-1; };
struct ItemRuntime {
    float health=100, maxHealth=100;
    std::vector<std::function<void()>> queue;
    float GetHealth(const char*,const char*){return health;}
    float GetMaxHealth(const char*,const char*){return maxHealth;}
    void DecreaseHealth(const char*,const char*,float damage){health=std::max(0.f,health-damage);}
    void OnStoreSave(Context c){c.Write(health);}
    bool OnStoreLoad(Context c,int){return c.Read(health);}
    void flush(){auto pending=std::move(queue);queue.clear();for(auto& f:pending)f();}
};
void near(float a,float b){if(std::abs(a-b)>0.002){std::cerr<<a<<" != "<<b<<"\n";std::abort();}}
'''
tests = r'''
int main() {
    ArPenArmorData armor;
    ItemBase item;
    near(item.ArPen_GetCurrentArmorHealth(armor),800);
    ArPenHitResult hit; hit.ArmorDamage=200;
    item.ArPen_ApplyDamage(armor,hit);
    near(item.health,100); // Native HP is intentionally deferred.
    near(item.ArPen_GetCurrentArmorHealth(armor),600); // No phantom repair.
    item.ArPen_ApplyDamage(armor,hit);
    near(item.ArPen_GetCurrentArmorHealth(armor),400);
    item.flush(); near(item.health,50);
    item.health=75; // Repair.
    near(item.ArPen_GetCurrentArmorHealth(armor),600);
    item.health=25; // External damage.
    near(item.ArPen_GetCurrentArmorHealth(armor),200);
    item.ArPen_ApplyDamage(armor,hit);
    near(item.ArPen_GetCurrentArmorHealth(armor),0); // Depleted before callbacks.
    item.flush(); near(item.health,0);
    ItemBase steel; steel.health=50;
    float failureVolume=armor.SurfaceAreaCM2*100*armor.ThicknessMM*.25;
    near(steel.ArPen_GetMetalLossVolumeMM3(armor)/failureVolume,.5);
    float nextLoss=steel.ArPen_GetMetalLossVolumeMM3(armor)+failureVolume*.01;
    float healthAfter=armor.BaseArmorHealth*(1-nextLoss/failureVolume);
    hit.ArmorDamage=steel.ArPen_GetCurrentArmorHealth(armor)-healthAfter;
    assert(hit.ArmorDamage>0);
    hit.AddedMetalLossVolumeMM3=failureVolume*.01;
    steel.ArPen_ApplyDamage(armor,hit);steel.flush();near(steel.health,49);
    steel.health=75; near(steel.ArPen_GetMetalLossVolumeMM3(armor)/failureVolume,.25);
    ArPenArmorData tiled;
    for(int i=0;i<16;i++)tiled.Tiles.Insert(800);
    ItemBase vest;
    near(vest.ArPen_GetCurrentArmorHealth(tiled),800);
    for(int tile=0;tile<4;tile++) {
        ArPenHitResult tileHit;tileHit.TileIndex=tile;tileHit.ArmorDamage=800;
        vest.ArPen_ApplyDamage(tiled,tileHit);
        near(vest.ArPen_GetTileHealth01(tiled,tile),0);
        if(tile<3)assert(vest.ArPen_GetCurrentArmorHealth(tiled)>0);
    }
    near(vest.ArPen_GetCurrentArmorHealth(tiled),0);
    vest.flush();near(vest.health,0);
    // Saving between callbacks must save matching visible/internal health.
    ItemBase saved;
    ArPenHitResult partial;partial.TileIndex=0;partial.ArmorDamage=400;
    saved.ArPen_ApplyDamage(tiled,partial);
    Context ctx;saved.OnStoreSave(ctx);
    assert(saved.queue.empty());near(saved.health,96.875);
    ItemBase loaded;assert(loaded.OnStoreLoad(ctx,0));
    near(loaded.ArPen_GetTileHealth01(tiled,0),.5);
    near(loaded.ArPen_GetTileHealth01(tiled,1),1);
    near(loaded.ArPen_GetCurrentArmorHealth(tiled),775);
    // Repair restores tile durability; next damage does not erase the repair.
    loaded.health=100;
    near(loaded.ArPen_GetTileHealth01(tiled,0),1);
    loaded.ArPen_ApplyDamage(tiled,partial);
    near(loaded.ArPen_GetTileHealth01(tiled,0),.5);
    loaded.flush();near(loaded.health,96.875);
    std::cout << "Production-state host regressions passed: pending hits, repairs, external damage, steel baseline, tiled failure, persistence.\n";
}
'''
with tempfile.TemporaryDirectory() as directory:
    cpp = Path(directory) / 'state.cpp'
    exe = Path(directory) / 'state-tests'
    cpp.write_text(prelude + source + tests)
    subprocess.run(['g++','-std=c++17','-O0','-Wall','-Wextra',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
