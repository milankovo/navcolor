#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

#define SET_FUNC_COLOR_ACTION "navcolor:change_color"

bool ask_color(bgcolor_t &color)
{
  static const char form[] =
      "Please pick a color\n"
      "\n"
      "<~C~olor:K::16::>\n";
  if (ask_form(form, &color) == 1)
    return true;
  return false;
}

struct ahandler_t : public action_handler_t
{

  ahandler_t() {}
  virtual int idaapi activate(action_activation_ctx_t *ctx) override
  {
    bgcolor_t color = DEFCOLOR;
    for (auto &n : ctx->chooser_selection)
    {
      auto fnc = getn_func(n);
      if (!fnc)
        continue;
      if (fnc->color != DEFCOLOR)
      {
        color = fnc->color;
        break;
      }
    }

    if (!ask_color(color))
      return 0;
    for (auto &n : ctx->chooser_selection)
    {
      auto fnc = getn_func(n);
      if (!fnc)
        continue;
      fnc->color = color;
      update_func(fnc);
    }
    return 1;
  }

  virtual action_state_t idaapi update(action_update_ctx_t *ctx) override
  {
    return ctx->widget_type == BWN_FUNCS ? AST_ENABLE_FOR_WIDGET : AST_DISABLE_FOR_WIDGET;
  }
};

static ahandler_t ah1;

//--------------------------------------------------------------------------
struct plugin_ctx_t : public plugmod_t, public event_listener_t
{
  nav_colorizer_t *old_col_fun = nullptr;
  void *old_col_ud = nullptr;
  bool installed = false;

  plugin_ctx_t()
  {
    // install our callback for navigation band
    install();

    const action_desc_t actions[] =
        {
#define ROW(name, label, handler) ACTION_DESC_LITERAL_PLUGMOD(name, label, handler, this, nullptr, nullptr, -1)
            ROW(SET_FUNC_COLOR_ACTION, "Set function color", &ah1),
#undef ROW
        };

    for (size_t i = 0, n = qnumber(actions); i < n; ++i)
      register_action(actions[i]);
    hook_event_listener(HT_UI, this);
  }
  // lint -esym(1540, plugin_ctx_t::old_col_fun, plugin_ctx_t::old_col_ud)
  ~plugin_ctx_t()
  {
    // uninstall our callback for navigation band, otherwise ida will crash
    uninstall();
  }
  virtual bool idaapi run(size_t) override;

  bool install();
  bool uninstall();

  virtual ssize_t idaapi on_event(ssize_t code, va_list va) override;
};

//--------------------------------------------------------------------------

ssize_t idaapi plugin_ctx_t::on_event(ssize_t code, va_list va)
{
  switch (code)
  {
  case ui_populating_widget_popup:
  {
    TWidget *view = va_arg(va, TWidget *);
    if (get_widget_type(view) == BWN_FUNCS)
    {
      TPopupMenu *p = va_arg(va, TPopupMenu *);
      attach_action_to_popup(view, p, SET_FUNC_COLOR_ACTION);
    }
  }
  break;
  }
  return 0;
};

//--------------------------------------------------------------------------
bool idaapi plugin_ctx_t::run(size_t code)
{
  if (installed)
    return uninstall();
  return install();
}

//--------------------------------------------------------------------------
// Callback that calculates the pixel color given the address and the number of bytes
static uint32 idaapi my_colorizer(ea_t ea, asize_t nbytes, void *ud)
{
  plugin_ctx_t &ctx = *(plugin_ctx_t *)ud;
  // try to get the color of the item
  {
    auto item_color = get_item_color(get_item_head(ea));
    if (item_color != DEFCOLOR)
      return item_color;
  }
  
  // else try to get the color of the function
  {
    auto fnc = get_func(ea);
    if (fnc && (fnc->color != DEFCOLOR))
      return fnc->color;
  }

  // else try to get the color of the segment
  {
    auto seg = getseg(ea);
    if (seg && seg->color != DEFCOLOR)
      return seg->color;
  }
  // else call the old colorizer
  uint32 color = ctx.old_col_fun(ea, nbytes, ctx.old_col_ud);
  return color;
}

//-------------------------------------------------------------------------
bool plugin_ctx_t::install()
{
  if (installed)
    return false;

  set_nav_colorizer(&old_col_fun, &old_col_ud, my_colorizer, this);
  installed = true;
  msg("Navigation band colorizer enabled\n");
  return true;
}

//-------------------------------------------------------------------------
bool plugin_ctx_t::uninstall()
{

  if (!installed)
    return 0;
  set_nav_colorizer(nullptr, nullptr, old_col_fun, old_col_ud);
  installed = false;
  msg("Navigation band colorizer disabled\n");
  return true;
}

//--------------------------------------------------------------------------
// initialize the plugin
static plugmod_t *idaapi init()
{
  return new plugin_ctx_t;
}

//--------------------------------------------------------------------------
plugin_t PLUGIN =
    {
        IDP_INTERFACE_VERSION,
        PLUGIN_MULTI, // The plugin can work with multiple idbs in parallel
        init,         // initialize
        nullptr,
        nullptr,
        nullptr,                         // long comment about the plugin
        nullptr,                         // multiline help about the plugin
        "Modify navigation band colors", // the preferred short name of the plugin
        nullptr,                         // the preferred hotkey to run the plugin
};
