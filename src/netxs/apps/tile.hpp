// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#ifndef NETXS_APP_TILE_HPP
#define NETXS_APP_TILE_HPP

namespace netxs::events::userland
{
    struct tile
    {
        EVENTPACK( tile, netxs::events::userland::root::custom )
        {
            GROUP_XS( ui, input::hids ), // Window manager command pack.

            SUBSET_XS( ui )
            {
                EVENT_XS( create  , input::hids ),
                EVENT_XS( close   , input::hids ),
                EVENT_XS( toggle  , input::hids ), // toggle window size: maximize/restore.
                EVENT_XS( swap    , input::hids ),
                EVENT_XS( rotate  , input::hids ), // change nested objects order. See tilimg manager (ui::fork).
                EVENT_XS( equalize, input::hids ),
                EVENT_XS( select  , input::hids ),
                GROUP_XS( split   , input::hids ),

                SUBSET_XS( split )
                {
                    EVENT_XS( vt, input::hids ),
                    EVENT_XS( hz, input::hids ),
                };
            };
        };
    };
}

namespace netxs::app
{
    // tile: Built-in tiling window manager.
    using tile = netxs::events::userland::tile;

    //todo revise
    using namespace netxs::console;
    using namespace netxs;
    using slot = ui::slot;
    using axis = ui::axis;
    using axes = ui::axes;
    using snap = ui::snap;
    using id_t = netxs::input::id_t;

    //todo unify
    const static auto term_menu_bg = rgba{ 0x80404040 };

    //todo unify
    auto build = [](auto create_app, auto window, view data)
    {
        #ifndef PROD
            if (app::shared::tile_count < APPS_MAX_COUNT)
            {
                auto c = &app::shared::tile_count; (*c)++;
                window->SUBMIT_BYVAL(tier::release, e2::dtor, item_id)
                        {
                            (*c)--;
                            log("main: tile manager destoyed");
                        };
            }
            else
            {
                create_app(create_app, window, Empty, "Reached the limit");
                break;
            }
        #endif

        view envvar_data;
        text window_title;
        auto a = data.find('=');
        if (a != text::npos)
        {
            auto b = data.begin();
            auto e = data.end();
            auto t = b + a;
            //auto envvar_name = view{ b, t }; //todo apple clang doesn't get it
            auto envvar_name = view{ &(*b), (size_t)(t - b) };
            log(" envvar_name=", envvar_name);
            b = t + 1;
            if (b != e)
            {
                //envvar_data = view{ b, e }; //todo apple clang doesn't get it
                envvar_data = view{ &(*b), (size_t)(e - b) };
                log(" envvar_data=", envvar_data);
                auto menu_name = utf::get_quote(envvar_data, '\"');
                window_title = utf::get_quote(envvar_data, '\"');
                log(" menu_name=", menu_name);
                log(" window_title=", window_title);
                utf::trim_front(envvar_data, ", ");
                log(" layout_data=", envvar_data);
                //if (window_title.length()) window_title += '\n';
            }
        }
        window->template unplug<pro::focus>() // Remove focus controller.
                ->invoke([&](auto& boss)
                {
                    boss.SUBMIT_BYVAL(tier::release, e2::form::upon::vtree::attached, parent)
                    {
                        auto title = ansi::add(window_title);// + utf::debase(data));
                        parent->base::riseup<tier::preview>(e2::form::prop::header, title);
                    };
                });


        auto object = window->attach(ui::fork::ctor(axis::Y));
            auto menu = object->attach(slot::_1, shared::custom_menu(true,
                std::list{
                        //  Green                                  ?Even    Red
                        // ┌────┐  ┌────┐  ┌─┬──┐  ┌────┐  ┌─┬──┐  ┌─┬──┐  ┌────┐  // ┌─┐  ┌─┬─┐  ┌─┬─┐  ┌─┬─┐  
                        // │Exec│  ├─┐  │  │ H  │  ├ V ─┤  │Swap│  │Fair│  │Shut│  // ├─┤  └─┴─┘  └<┴>┘  └>┴<┘  
                        // └────┘  └─┴──┘  └─┴──┘  └────┘  └─┴──┘  └─┴──┘  └────┘  // └─┘                       
                        std::pair<text, std::function<void(ui::pads&)>>{"  ┐└  ",//  ─┐  ", //"  ▀█  ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                gear.countdown = 1;
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::toggle, gear);
                                //iota status = 1;
                                //boss.base::broadcast->SIGNAL(tier::request, e2::command::custom, status);
                                //boss.base::broadcast->SIGNAL(tier::preview, e2::command::custom, status == 2 ? 1/*show*/ : 2/*hide*/);
                                gear.dismiss(true);
                            };
                            boss.base::broadcast->SUBMIT(tier::release, e2::command::custom, status)
                            {
                                //boss.color(status == 1 ? 0xFF00ff00 : x3.fgc(), x3.bgc());
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ "  +  ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::create, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ " ::: ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::select, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ "  │  ", // "  ║  ", - VGA Linux console doesn't support unicode glyphs
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::split::hz, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{  " ── ", // " ══ ", - VGA Linux console doesn't support unicode glyphs
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::split::vt, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ "  ┌┘  ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::rotate, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ " <-> ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::swap, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ " >|< ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::equalize, gear);
                                gear.dismiss(true);
                            };
                        }},
                        std::pair<text, std::function<void(ui::pads&)>>{ "  ×  ",
                        [](ui::pads& boss)
                        {
                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                            {
                                boss.base::broadcast->SIGNAL(tier::preview, app::tile::ui::close, gear);
                                gear.dismiss(true);
                            };
                        }},
                    }))
                    ->colors(whitelt, term_menu_bg)
                    ->template plugin<pro::track>()
                    ->template plugin<pro::acryl>();

        auto mouse_actions = [](auto& boss)
        {
            boss.broadcast->SUBMIT_T(tier::preview, app::tile::ui::any, boss.tracker, gear)
            {
                auto gear_id = gear.id;
                boss.broadcast->SIGNAL(tier::request, e2::form::state::keybd::find, gear_id);
                if (gear_id)
                {
                    if (auto deed = boss.broadcast->bell::protos<tier::preview>())
                    {
                        switch (deed)
                        {
                            case app::tile::ui::create.id:
                                boss.template riseup<tier::release>(e2::form::proceed::createby, gear);
                                break;
                            case app::tile::ui::close.id:
                                boss.template riseup<tier::release>(e2::form::quit, boss.This());
                                break;
                            case app::tile::ui::toggle.id:
                                if (gear.countdown > 0)
                                {
                                    gear.countdown--;
                                    boss.template riseup<tier::release>(e2::form::maximize, gear);
                                }
                                break;
                            case app::tile::ui::swap.id:
                                boss.template riseup<tier::release>(app::tile::ui::swap, gear);
                                break;
                            case app::tile::ui::rotate.id:
                                boss.template riseup<tier::release>(app::tile::ui::rotate, gear);
                                break;
                            case app::tile::ui::equalize.id:
                                boss.template riseup<tier::release>(app::tile::ui::equalize, gear);
                                break;
                            case app::tile::ui::split::vt.id:
                                boss.template riseup<tier::release>(app::tile::ui::split::vt, gear);
                                break;
                            case app::tile::ui::split::hz.id:
                                boss.template riseup<tier::release>(app::tile::ui::split::hz, gear);
                                break;
                        }
                    }
                }
            };
            boss.SUBMIT(tier::release, hids::events::mouse::button::dblclick::left, gear)
            {
                boss.base::template riseup<tier::release>(e2::form::maximize, gear);
                gear.dismiss();
            };
            boss.SUBMIT(tier::release, hids::events::mouse::button::click::leftright, gear)
            {
                boss.base::template riseup<tier::release>(e2::form::quit, boss.This());
                gear.dismiss();
            };
            boss.SUBMIT(tier::release, hids::events::mouse::button::click::middle, gear)
            {
                boss.base::template riseup<tier::release>(e2::form::quit, boss.This());
                gear.dismiss();
            };
        };
        auto select_all = [](auto& boss)
        {
            boss.broadcast->SUBMIT_T(tier::preview, app::tile::ui::select, boss.tracker, gear)
            {
                //todo unify
                gear.force_group_focus = true;
                gear.kb_focus_taken = faux;
                gear.combine_focus = true;
                boss.SIGNAL(tier::release, hids::events::upevent::kboffer, gear);
                gear.combine_focus = faux;
                gear.force_group_focus = faux;
            };
        };
        auto box_with_title = [&](view title, auto branch)
        {
            return ui::fork::ctor(axis::Y)
                    ->plugin<pro::title>("", true, faux, true)
                    ->plugin<pro::limit>(twod{ 10,-1 }, twod{ -1,-1 })
                    ->isroot(true)
                    ->active()
                    ->invoke([&](auto& boss)
                    {
                        mouse_actions(boss);
                        select_all(boss);
                    })
                    //->branch(slot::_1, ui::post_fx<cell::shaders::contrast>::ctor()) //todo apple clang doesn't get it
                    ->branch(slot::_1, ui::post_fx::ctor()
                        ->upload(title)
                        ->invoke([&](auto& boss)
                        {
                            auto shadow = ptr::shadow(boss.This());
                            boss.SUBMIT_BYVAL(tier::release, e2::form::upon::vtree::attached, parent)
                            {
                                parent->SUBMIT_BYVAL(tier::preview, e2::form::prop::header, newtext)
                                {
                                    if (auto ptr = shadow.lock()) ptr->upload(newtext);
                                };
                                parent->SUBMIT_BYVAL(tier::request, e2::form::prop::header, curtext)
                                {
                                    if (auto ptr = shadow.lock()) curtext = ptr->get_source();
                                };
                            };
                        }))
                    ->branch(slot::_2, branch
                        ->invoke([&](auto& boss)
                        {
                            boss.base::broadcast->SIGNAL(tier::release, e2::form::prop::menusize, 1);
                        }));
        };
        auto pass_focus = [](auto& gear_id_list, auto& item_ptr)
        {
            if (item_ptr)
            {
                for (auto gear_id : gear_id_list)
                {
                    if (auto gate_ptr = bell::getref(gear_id))
                    {
                        gate_ptr->SIGNAL(tier::preview, e2::form::proceed::focus, item_ptr);
                    }
                }
            }
        };
        auto built_node = [&](auto tag, auto s1, auto s2, auto w)
        {
            auto node = tag == 'h' ? ui::fork::ctor(axis::X, w == -1 ? 2 : w, s1, s2)
                                    : ui::fork::ctor(axis::Y, w == -1 ? 1 : w, s1, s2);
            node->isroot(true, 1) // Set object kind to 1 to be different from others.
                ->invoke([&](auto& boss)
                {
                    mouse_actions(boss);
                    boss.SUBMIT(tier::release, app::tile::ui::swap    , gear) { boss.swap();       };
                    boss.SUBMIT(tier::release, app::tile::ui::rotate  , gear) { boss.rotate();     };
                    boss.SUBMIT(tier::release, app::tile::ui::equalize, gear) { boss.config(1, 1); };
                });
                auto grip = node->attach(slot::_I, ui::mock::ctor())
                                ->template plugin<pro::mover>()
                                ->template plugin<pro::focus>()
                                //->template plugin<pro::shade<cell::shaders::xlight>>() //todo apple clang doesn't get it
                                ->template plugin<pro::shade>()
                                ->invoke([&](auto& boss)
                                {
                                    boss.keybd.accept(true);
                                    //todo implement
                                })
                                ->active();
            return node;
        };
        auto place_holder = [&]()
        {
            return ui::park::ctor()
                ->isroot(true)
                ->colors(blacklt, term_menu_bg)
                ->template plugin<pro::limit>(dot_00, -dot_11)
                ->template plugin<pro::focus>()
                ->invoke([&](auto& boss)
                {
                    boss.keybd.accept(true);
                    mouse_actions(boss);
                    select_all(boss);
                    boss.SUBMIT(tier::release, hids::events::mouse::button::click::right, gear)
                    {
                        boss.base::template riseup<tier::release>(e2::form::proceed::createby, gear);
                        gear.dismiss();
                    };
                })
                ->branch
                (
                    snap::center, snap::center,
                    ui::post::ctor()->upload("Empty Slot", 10)
                );
        };
        auto empty_slot = [&](auto&& empty_slot) -> sptr<ui::veer>
        {
            return ui::veer::ctor()
                ->invoke([&](auto& boss)
                {
                    auto shadow = ptr::shadow(boss.This());
                    boss.SUBMIT(tier::release, e2::form::proceed::swap, item_ptr)
                    {
                        auto count = boss.count();
                        if (count == 1) // Only empty slot available.
                        {
                            log(" empty_slot swap: defective structure, count=", count);
                        }
                        else if (count == 2)
                        {
                            auto my_item_ptr = boss.pop_back();
                            if (item_ptr)
                            {
                                boss.attach(item_ptr);
                            }
                            else item_ptr = boss.This(); // Heir to the focus.
                        }
                        else log(" empty_slot swap: defective structure, count=", count);
                    };
                    boss.SUBMIT(tier::release, e2::form::upon::vtree::attached, parent)
                    {
                        //todo revise, possible parent subscription leaks when reattached
                        auto parent_memo = std::make_shared<subs>();
                        parent->SUBMIT_T(tier::request, e2::form::proceed::swap, *parent_memo, item_ptr)
                        {
                            if (item_ptr != boss.This()) // It wasn't me. It was the one-armed man.
                            {
                                auto count = boss.count();
                                if (count == 1) // Only empty slot available.
                                {
                                    item_ptr.reset();
                                }
                                else if (count == 2)
                                {
                                    item_ptr = boss.pop_back();
                                }
                                else log(" empty_slot: defective structure, count=", count);
                                if (auto parent = boss.parent())
                                {
                                    parent->bell::template expire<tier::request>();
                                }
                            }
                        };
                        boss.SUBMIT_T_BYVAL(tier::request, e2::form::upon::vtree::detached, *parent_memo, parent)
                        {
                            parent_memo.reset();
                        };
                    };
                    boss.broadcast->SUBMIT_T(tier::preview, app::tile::ui::any, boss.tracker, gear)
                    {
                        if (auto deed = boss.broadcast->bell::template protos<tier::preview>())
                        {
                            //auto size = boss.count();
                            //if (deed == app::tile::ui::toggle.id
                            // && size > 2
                            // && gear.countdown > 0)
                            //{
                            //    if (auto fullscreen_item = boss.back())
                            //    {
                            //        gear.countdown--;
                            //        fullscreen_item->SIGNAL(tier::release, app::tile::ui::toggle, gear);
                            //    }
                            //}
                            //else
                            if (auto item_ptr = boss.back())
                            {
                                auto& item = *item_ptr;
                                if (item.base::root())
                                {
                                    gear.force_group_focus = true;
                                    item.broadcast->template signal<tier::preview>(deed, gear);
                                    gear.force_group_focus = faux;
                                }
                            }
                        }
                    };
                    boss.SUBMIT_BYVAL(tier::release, e2::form::maximize, gear)
                    {
                        if (auto boss_ptr = shadow.lock())
                        {
                            auto& boss =*boss_ptr;
                            auto count = boss.count();
                            if (count > 1) // Preventing the empty slot from maximizing.
                            {
                                //todo revise
                                if (boss.back()->base::kind() == 0) // Preventing the splitter from maximizing.
                                {
                                    auto fullscreen_item = boss.pop_back();
                                    if (fullscreen_item)
                                    {
                                        // One-time return ticket.
                                        auto oneoff = std::make_shared<hook>();
                                        fullscreen_item->SUBMIT_T_BYVAL(tier::release, e2::form::maximize, *oneoff, gear)
                                        {
                                            if (auto boss_ptr = shadow.lock())
                                            {
                                                auto& boss = *boss_ptr;
                                                using type = decltype(e2::form::proceed::detach)::type;
                                                type fullscreen_item;
                                                boss.base::template riseup<tier::release>(e2::form::proceed::detach, fullscreen_item);
                                                if (fullscreen_item)
                                                {
                                                    boss.attach(fullscreen_item);
                                                    boss.base::reflow();
                                                }
                                            }
                                            oneoff.reset();
                                        };
                                        boss.base::template riseup<tier::release>(e2::form::proceed::attach, fullscreen_item);
                                        if (fullscreen_item) // Unsuccessful maximization. Attach it back.
                                        {
                                            boss.attach(fullscreen_item);
                                        }
                                        boss.base::reflow();
                                    }
                                }
                            }
                        }
                    };
                    boss.SUBMIT_BYVAL(tier::release, app::tile::ui::split::any, gear)
                    {
                        if (auto boss_ptr = shadow.lock())
                        {
                            auto& boss = *boss_ptr;
                            if (auto deed = boss.bell::template protos<tier::release>())
                            {
                                if (auto gate_ptr = bell::getref(gear.id))
                                {
                                    using type = decltype(e2::depth)::type;
                                    auto depth = type{};
                                    boss.base::template riseup<tier::request>(e2::depth, depth, true);
                                    log(" depth=", depth);
                                    if (depth > INHERITANCE_LIMIT) return;

                                    if (boss.back()->base::root())
                                    {
                                        auto heading = deed == app::tile::ui::split::vt.id;
                                        auto newnode = built_node(heading ? 'v':'h', 1, 1, heading ? 1 : 2);
                                        auto empty_1 = empty_slot(empty_slot);
                                        auto empty_2 = empty_slot(empty_slot);
                                        auto curitem = boss.pop_back(); // In order to preserve all foci.
                                        gate_ptr->SIGNAL(tier::preview, e2::form::proceed::focus,   empty_1);
                                        gate_ptr->SIGNAL(tier::preview, e2::form::proceed::unfocus, curitem);
                                        if (boss.empty())
                                        {
                                            boss.attach(place_holder());
                                            empty_2->pop_back();
                                        }
                                        auto slot_1 = newnode->attach(slot::_1, empty_1);
                                        auto slot_2 = newnode->attach(slot::_2, empty_2->branch(curitem));
                                        boss.attach(newnode);
                                    }
                                    else log(" empty_slot split: defective structure, count=", boss.count());
                                }
                            }
                        }
                    };
                    boss.SUBMIT_BYVAL(tier::release, e2::form::quit, nested_item_ptr)
                    {
                        if (nested_item_ptr)
                        {
                            auto& item = *nested_item_ptr;
                            using type = decltype(e2::form::state::keybd::handover)::type;
                            type gear_id_list;
                            item.broadcast->SIGNAL(tier::request, e2::form::state::keybd::handover, gear_id_list);

                            if (auto boss_ptr = shadow.lock())
                            {
                                auto& boss = *boss_ptr;
                                auto count = boss.count();
                                if (count > 1)
                                {
                                    if (boss.back()->base::kind() == 0) // Only apps can be deleted.
                                    {
                                        auto item = boss.pop_back(); // Throw away.
                                        pass_focus(gear_id_list, boss_ptr);
                                    }
                                }
                                else if (count == 1) // Remove empty slot, reorganize.
                                {
                                    if (auto parent = boss.base::parent())
                                    {
                                        using type = decltype(e2::form::proceed::swap)::type;
                                        type item_ptr = boss_ptr; // sptr must be of the same type as the event argument. Casting kills all intermediaries when return.
                                        parent->SIGNAL(tier::request, e2::form::proceed::swap, item_ptr);
                                        if (item_ptr)
                                        {
                                            if (item_ptr != boss_ptr) // Parallel slot is not empty.
                                            {
                                                parent->base::template riseup<tier::release>(e2::form::proceed::swap, item_ptr);
                                                pass_focus(gear_id_list, item_ptr);
                                            }
                                            else // I'm alone.
                                            {
                                                // Nothing todo. There can be only one!
                                            }
                                        }
                                        else // Both slots are empty.
                                        {
                                            parent->base::template riseup<tier::release>(e2::form::proceed::swap, item_ptr);
                                            pass_focus(gear_id_list, item_ptr);
                                        }
                                    }
                                }
                            }
                        }
                    };
                    boss.SUBMIT(tier::release, e2::form::proceed::createby, gear)
                    {
                        static iota insts_count = 0;
                        if (boss.count() == 1) // Create new apps at the empty slots only.
                        {
                            if (gear.meta(hids::ANYCTRL))
                            {
                                //todo ...
                            }
                            else
                            {
                                if (auto gate_ptr = bell::getref(gear.id))
                                {
                                    auto& gate = *gate_ptr;
                                    auto data = decltype(e2::data::changed)::type{};
                                    gate.SIGNAL(tier::request, e2::data::changed, data);
                                    auto current_default = static_cast<id_t>(data);
                                    auto config = app::shared::objs_config[current_default];

                                    auto host = ui::cake::ctor()
                                                ->plugin<pro::focus>();
                                    create_app(create_app, host, current_default, config.data);
                                    auto app = box_with_title(config.title, host);
                                    gear.remove_from_kb_focus(boss.back()); // Take focus from the empty slot.
                                    boss.attach(app);

                                    //todo unify, demo limits
                                    {
                                        insts_count++;
                                        #ifndef PROD
                                            if (insts_count > APPS_MAX_COUNT)
                                            {
                                                log("tile: inst: max count reached");
                                                auto timeout = tempus::now() + APPS_DEL_TIMEOUT;
                                                auto w_frame = ptr::shadow(host);
                                                host->SUBMIT_BYVAL(tier::general, e2::tick, timestamp)
                                                {
                                                    if (timestamp > timeout)
                                                    {
                                                        log("tile: inst: timebomb");
                                                        if (auto host = w_frame.lock())
                                                        {
                                                            host->riseup<tier::release>(e2::form::quit, host);
                                                            //host->base::detach();
                                                            log("tile: inst: frame detached: ", insts_count);
                                                        }
                                                    }
                                                };
                                            }
                                        #endif
                                        host->SUBMIT(tier::release, events::userland::root::dtor, id)
                                        {
                                            insts_count--;
                                            log("tile: inst: detached: ", insts_count, " id=", id);
                                        };
                                    }
                                    //todo unify
                                    gear.kb_focus_taken = faux;
                                    host->SIGNAL(tier::release, hids::events::upevent::kboffer, gear);
                                }
                            }
                        }
                    };
                })
                ->branch
                (
                    place_holder()
                );
        };
        auto add_node = [&](auto&& add_node, view& utf8) -> sptr<ui::veer>
        {
            auto place = empty_slot(empty_slot);
            utf::trim_front(utf8, ", ");
            if (utf8.empty()) return place;
            auto tag = utf8.front();
            if (tag == '\"')
            {
                // add term
                auto cmdline = utf::get_quote(utf8, '\"');
                log(" node cmdline=", cmdline);
                auto host = ui::cake::ctor()
                        ->plugin<pro::focus>();
                create_app(create_app, host, app::shared::objs_map["Term"], cmdline);
                auto inst = box_with_title("Headless TE", host);

                // empty_slot<veer>->(r0,f)place_holder<park>
                //                 ->(r0)box_with_title<fork>->title
                //                                           ->(f)cake->app
                // empty_slot<veer>->(r0,f)place_holder<park>
                //                 ->(r1)node_split<fork>->slot_1 ...
                //                                       ->slot_2 ...
                //                                       ->(f)grip

                place->attach(inst);
            }
            else if (tag == 'a')
            {
                // add app
                utf8.remove_prefix(1);
                utf::trim_front(utf8, " ");
                if (utf8.empty() || utf8.front() != '(') return place;
                utf8.remove_prefix(1);
                auto app_id  = utf::get_quote(utf8, '\"');
                utf::trim_front(utf8, ", ");
                auto app_title = utf::get_quote(utf8, '\"');
                utf::trim_front(utf8, ", ");
                auto app_data = utf::get_quote(utf8, '\"');
                log(" app_id=", app_id, " app_title=", app_title, " app_data=", app_data);
                auto host = ui::cake::ctor()
                        ->plugin<pro::focus>();
                create_app(create_app, host, app::shared::objs_map[app_id], app_data);
                auto app = box_with_title(app_title, host);
                place->attach(app);
                utf::trim_front(utf8, ") ");
            }
            else if (tag == 'h' || tag == 'v')
            {
                // add split
                utf8.remove_prefix(1);
                utf::trim_front(utf8, " ");
                iota s1 = 1;
                iota s2 = 1;
                iota w = -1;
                if (auto param = utf::to_int(utf8))
                {
                    s1 = std::abs(param.value());
                    if (utf8.empty() || utf8.front() != ':') return place;
                    utf8.remove_prefix(1);
                    if (auto param = utf::to_int(utf8))
                    {
                        s2 = std::abs(param.value());
                        utf::trim_front(utf8, " ");
                        if (!utf8.empty() && utf8.front() == ':') // Grip width.
                        {
                            utf8.remove_prefix(1);
                            if (auto param = utf::to_int(utf8))
                            {
                                w = std::abs(param.value());
                                utf::trim_front(utf8, " ");
                            }
                        }
                    }
                    else return place;
                }
                if (utf8.empty() || utf8.front() != '(') return place;
                utf8.remove_prefix(1);
                auto node = built_node(tag, s1, s2, w);
                auto slot1 = node->attach(slot::_1, add_node(add_node, utf8));
                auto slot2 = node->attach(slot::_2, add_node(add_node, utf8));
                place->attach(node);

                utf::trim_front(utf8, ") ");
            }
            return place;
        };
        auto host = object->attach(slot::_2, add_node(add_node, envvar_data));
        host->invoke([&](auto& boss)
        {
            boss.SUBMIT(tier::release, e2::form::proceed::attach, fullscreen_item)
            {
                if (fullscreen_item)
                {
                    boss.attach(fullscreen_item);
                    fullscreen_item.reset();
                }
            };
            boss.SUBMIT(tier::release, e2::form::proceed::detach, fullscreen_item)
            {
                auto item = boss.pop_back();
                if (item) fullscreen_item = item;
            };
            boss.SUBMIT(tier::release, e2::form::upon::created, gear)
            {
                if (auto gate_ptr = bell::getref(gear.id))
                {
                    auto& gate = *gate_ptr;
                    auto menu_item_id = decltype(e2::data::changed)::type{};
                    gate.SIGNAL(tier::request, e2::data::changed, menu_item_id);

                    auto config = app::shared::objs_config[menu_item_id];
                    if (config.name == "Tile") // Reset the currently selected application to the previous one.
                    {
                        gate.SIGNAL(tier::preview, e2::data::changed, menu_item_id); // Get previous default;
                        gate.SIGNAL(tier::release, e2::data::changed, menu_item_id); // Set current  default;
                    }
                }
            };
        });
    };
}

#endif // NETXS_APP_TILE_HPP