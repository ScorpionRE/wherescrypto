/*
 *      Hex-Rays Decompiler project
 *      Copyright (c) 2007-2020 by Hex-Rays, support@hex-rays.com
 *      ALL RIGHTS RESERVED.
 *
 *      Sample plugin for Hex-Rays Decompiler.
 *      It generates microcode for selection and dumps it to the output window.
 */

#include <hexrays.hpp>
#include <frame.hpp>


 // Hex-Rays API pointer
hexdsp_t* hexdsp = NULL;

//--------------------------------------------------------------------------
struct plugin_ctx_t : public plugmod_t
{
    ~plugin_ctx_t()
    {
        if (hexdsp != nullptr)
            term_hexrays_plugin();
    }
    virtual bool idaapi run(size_t) override;
};

struct  top_visitor_t : public minsn_visitor_t
{
    int visit_minsn() {
        minsn_t* ins = this->curins;

    }
};


//--------------------------------------------------------------------------
bool idaapi plugin_ctx_t::run(size_t)
{
    hexrays_failure_t hf;
    mba_ranges_t mbr = mba_ranges_t();
    func_t* fn = get_func(get_screen_ea());
    if (fn == NULL)
    {
        warning("Please position the cursor within a function");
        return true;
    }
 
    ea_t ea1 = fn->start_ea;
    ea_t ea2 = fn->end_ea;
    hf = hexrays_failure_t();
 
    flags_t F = get_flags(ea1);
    if (!is_code(F))
    {
        warning("The selected range must start with an instruction");
        return true;
    }
    mbr.ranges.push_back(range_t(ea1, ea2));
    mba_t* mba = gen_microcode(mbr, &hf, NULL, DECOMP_WARNINGS);
    if (mba == NULL)
    {
        warning("%a: %s", hf.errea, hf.desc().c_str());
        return true;
    }

    msg("Successfully generated microcode for %a..%a\n", ea1, ea2);
    // vd_printer_t vp;
    // mba->print(vp);

    // basic blocks

    int qty = mba->qty;
    mblock_t *blocks = mba->blocks->nextb;
    msg("%d basic blocks", qty);

    
        // instructions 
        msg("No %d basic block", blocks->serial);
        minsn_t* ins = blocks->head;
       
        msg("instructs opcode:%x, l:%d.%d  , r:%d.%d , d: %d.%d ", ins->opcode, ins->l.r, ins->l.size, ins->r.r, ins->r.size, ins->d.r, ins->d.size);
         
        
    
    
    // We must explicitly delete the microcode array
    delete mba;
    return true;
}

//--------------------------------------------------------------------------
static plugmod_t* idaapi init()
{
    if (!init_hexrays_plugin())
        return nullptr; // no decompiler
    const char* hxver = get_hexrays_version();
    msg("Hex-rays version %s has been detected, %s ready to use\n",
        hxver, PLUGIN.wanted_name);
    return new plugin_ctx_t;
}

//--------------------------------------------------------------------------
static const char comment[] = "Sample13 plugin for Hex-Rays decompiler";

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_MULTI,         // The plugin can work with multiple idbs in parallel
  init,                 // initialize
  nullptr,
  nullptr,
  comment,              // long comment about the plugin
  nullptr,              // multiline help about the plugin
  "Dump microcode for selected range", // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
