// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

#include "netxs/desktopio/application.hpp"
#include "netxs/apps/desk.hpp"

namespace netxs::app::vtm
{
    static constexpr auto id = "vtm";
    using ui::sptr;
    using ui::wptr;

    struct link
    {
        text menuid{};
        text kindid{};
        text header{};
        text footer{};
        rect square{};
        bool forced{};
        sptr applet{};
    };

    namespace attr
    {
        static constexpr auto id       = "id";
        static constexpr auto alias    = "alias";
        static constexpr auto hidden   = "hidden";
        static constexpr auto label    = "label";
        static constexpr auto notes    = "notes";
        static constexpr auto title    = "title";
        static constexpr auto footer   = "footer";
        static constexpr auto bgc      = "bgc";
        static constexpr auto fgc      = "fgc";
        static constexpr auto winsize  = "winsize";
        static constexpr auto wincoor  = "wincoor";
        static constexpr auto focused  = "focused";
        static constexpr auto slimmenu = "slimmenu";
        static constexpr auto hotkey   = "hotkey";
        static constexpr auto type     = "type";
        static constexpr auto cwd      = "cwd";
        static constexpr auto param    = "param";
        static constexpr auto splitter = "splitter";
        static constexpr auto config   = "config";
    }
    namespace path
    {
        static constexpr auto item     = "/config/menu/item";
        static constexpr auto autorun  = "/config/menu/autorun/item";
        static constexpr auto viewport = "/config/menu/viewport/coor";
    }

    struct events
    {
        EVENTPACK( events, ui::e2::extra::slot1 )
        {
            EVENT_XS( newapp  , link ), // request: create new object using specified meniid.
            EVENT_XS( apptype , link ), // request: ask app type.
            EVENT_XS( handoff , link ), // general: attach spcified intance and return sptr.
            EVENT_XS( attached, sptr ), // anycast: inform that the object tree is attached to the world.
            GROUP_XS( d_n_d   , sptr ), // drag&drop functionality. See tiling manager empty slot and pro::d_n_d.
            GROUP_XS( gate    , sptr ),

            SUBSET_XS(d_n_d)
            {
                EVENT_XS( ask  , sptr ),
                EVENT_XS( abort, sptr ),
                EVENT_XS( drop , link ),
            };
            SUBSET_XS(gate)
            {
                EVENT_XS( fullscreen, link ), // release: Toggle fullscreen mode.
                EVENT_XS( restore   , link ), // release: Restore from fullscreen.
            };
        };
    };

    namespace pro
    {
        using namespace netxs::ui::pro;

        // pro: Fullscreen size-binding functionality.
        class align
            : public skill
        {
            using skill::boss,
                  skill::memo;

            enum class type { full, size, coor, };

        public:
            //todo revise
            wptr& nexthop;
            wptr saved;
            link what; // align: Original window properties.
            rect prev; // align: Window size before the fullscreen has applied.
            twod coor; // align: Coor tracking.
            hook maxs; // align: Fullscreen event subscription token.

            align(base&&) = delete;
            align(base& boss, wptr& nexthop, bool maximize = true)
                : skill{ boss },
                  nexthop{ nexthop}
            {
                boss.LISTEN(tier::release, vtm::events::gate::fullscreen, new_what, maxs)
                {
                    auto is_new = what.applet != new_what.applet;
                    if (what.applet) unbind();
                    if (is_new) follow(new_what);
                };
            }
           ~align()
            {
                if (what.applet) unbind();
            }

            void follow(vtm::link& new_what, dent pads = {})
            {
                what = new_what;
                auto window_ptr = new_what.applet;
                auto gear_id_list = pro::focus::get(window_ptr, true); // Expropriate all foci.
                saved = nexthop;
                nexthop = new_what.applet;
                window_ptr->base::detach();
                prev = window_ptr->base::area();
                auto new_pos = boss.base::area() + pads;
                new_pos.coor -= boss.base::coor();
                window_ptr->base::extend(new_pos);
                coor = window_ptr->base::coor();

                auto newhead = std::move(what.header);
                auto newfoot = std::move(what.footer);
                boss.RISEUP(tier::request, e2::form::prop::ui::header, what.header);
                boss.RISEUP(tier::request, e2::form::prop::ui::footer, what.footer);
                boss.RISEUP(tier::preview, e2::form::prop::ui::header, newhead);
                boss.RISEUP(tier::preview, e2::form::prop::ui::footer, newfoot);

                boss.LISTEN(tier::anycast, e2::form::proceed::quit::one, fast, memo)
                {
                    unbind();
                };
                boss.LISTEN(tier::release, e2::area, new_area, memo, (pads))
                {
                    if (new_area.coor != boss.base::coor()) unbind();
                    else what.applet->base::resize(new_area.size + pads);
                };
                window_ptr->LISTEN(tier::release, e2::form::layout::minimize, gear, memo)
                {
                    what.applet->bell::expire<tier::release>(); // Suppress hide/minimization.
                    unbind();
                };
                window_ptr->LISTEN(tier::release, e2::form::proceed::quit::one, fast, memo)
                {
                    unbind();
                    boss.expire<tier::release>(true);
                };
                window_ptr->LISTEN(tier::release, e2::area, new_area, memo)
                {
                    if (coor != new_area.coor) unbind(type::size);
                };

                window_ptr->SIGNAL(tier::release, e2::form::upon::vtree::attached, boss.base::This());
                window_ptr->SIGNAL(tier::anycast, vtm::events::attached, boss.base::This());
                pro::focus::set(window_ptr, gear_id_list, pro::focus::solo::on, pro::focus::flip::off, true); // Refocus.
            }
            void unbind(type restore = type::full)
            {
                if (!memo) return;
                nexthop = saved;
                saved.reset();
                memo.clear();
                auto prev_header = std::move(what.header);
                auto prev_footer = std::move(what.footer);
                boss.RISEUP(tier::request, e2::form::prop::ui::header, what.header);
                boss.RISEUP(tier::request, e2::form::prop::ui::footer, what.footer);
                boss.RISEUP(tier::preview, e2::form::prop::ui::header, prev_header);
                boss.RISEUP(tier::preview, e2::form::prop::ui::footer, prev_footer);
                boss.SIGNAL(tier::anycast, e2::form::layout::restore, empty, ()); // Notify app::desk to suppress triggering.
                auto window_ptr = what.applet;
                auto gear_id_list = pro::focus::get(window_ptr, true); // Expropriate all foci.
                window_ptr->base::detach();
                if (auto world_ptr = boss.base::parent())
                {
                    world_ptr->SIGNAL(tier::release, vtm::events::gate::restore, what);
                }
                switch (restore)
                {
                    case type::full: window_ptr->base::extend(prev); break; // Restore previous position.
                    case type::coor: window_ptr->base::moveto(prev.coor); break;
                    case type::size: window_ptr->base::resize(prev.size);
                                     window_ptr->base::moveby(boss.base::coor() + window_ptr->base::anchor - prev.size / twod{ 2,4 }); // Centrify on mouse. See pro::frame pull.
                                     break;
                }
                what.applet.reset();
                pro::focus::set(window_ptr, gear_id_list, pro::focus::solo::on, pro::focus::flip::off, true); // Refocus.
            }
        };

        // pro: Provides functionality for manipulating objects with a frame structure.
        class frame
            : public skill
        {
            using skill::boss,
                  skill::memo;

            robot robo;
            zpos  seat;

        public:
            frame(base&&) = delete;
            frame(base& boss, zpos z_order = zpos::plain) : skill{ boss },
                robo{ boss    },
                seat{ z_order }
            {
                boss.LISTEN(tier::release, e2::form::upon::vtree::attached, parent, memo)
                {
                    parent->LISTEN(tier::preview, e2::form::global::lucidity, alpha, boss.relyon)
                    {
                        boss.SIGNAL(tier::preview, e2::form::global::lucidity, alpha);
                    };
                    parent->LISTEN(tier::preview, e2::form::layout::convey, convey_data, boss.relyon)
                    {
                        convey(convey_data.delta, convey_data.stuff);
                    };
                    parent->LISTEN(tier::preview, e2::form::layout::shift, delta, boss.relyon)
                    {
                        boss.moveby(delta);
                    };
                    boss.SIGNAL(tier::release, e2::form::prop::zorder, seat);
                };
                boss.LISTEN(tier::preview, e2::form::prop::zorder, order)
                {
                    seat = order;
                    boss.SIGNAL(tier::release, e2::form::prop::zorder, seat);
                };
                boss.LISTEN(tier::request, e2::form::prop::zorder, order)
                {
                    order = seat;
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::click::left, gear, memo)
                {
                    boss.RISEUP(tier::preview, e2::form::layout::expose, area, ());
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::click::right, gear, memo)
                {
                    boss.RISEUP(tier::preview, e2::form::layout::expose, area, ());
                };
                boss.LISTEN(tier::preview, e2::form::layout::appear, newpos, memo)
                {
                    appear(newpos);
                };
                //boss.LISTEN(tier::preview, e2::form::upon::moved, delta, memo)
                //{
                //    bubble();
                //};
                boss.LISTEN(tier::preview, e2::form::upon::changed, delta, memo)
                {
                    boss.RISEUP(tier::preview, e2::form::layout::bubble, area, ());
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::down::any, gear, memo)
                {
                    robo.pacify();
                };
                boss.LISTEN(tier::release, e2::form::drag::pull::any, gear, memo)
                {
                    if (gear)
                    {
                        auto deed = boss.bell::template protos<tier::release>();
                        switch (deed)
                        {
                            case e2::form::drag::pull::left.id:
                            case e2::form::drag::pull::leftright.id:
                            {
                                auto delta = gear.delta.get();
                                boss.base::anchor = gear.coord; // See pro::align unbind.
                                boss.base::moveby(delta);
                                boss.SIGNAL(tier::preview, e2::form::upon::changed, delta);
                                gear.dismiss();
                                break;
                            }
                            default: break;
                        }
                    }
                };
                boss.LISTEN(tier::release, e2::form::upon::dragged, gear, memo)
                {
                    if (gear.meta(hids::anyCtrl))
                    {
                        robo.actify(gear.fader<quadratic<twod>>(2s), [&](auto x)
                        {
                            boss.base::moveby(x);
                            boss.strike();
                        });
                    }
                    else
                    {
                        auto boundary = gear.area();
                        robo.actify(gear.fader<quadratic<twod>>(2s), [&, boundary](auto x)
                        {
                            convey(x, boundary);
                            boss.strike();
                        });
                    }
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::click::right, gear, memo)
                {
                    auto& area = boss.base::area();
                    auto coord = gear.coord + area.coor;
                    if (!area.hittest(coord))
                    {
                        pro::focus::set(boss.This(), gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        appear(coord);
                    }
                    gear.dismiss();
                };
            };

            // pro::frame: Fly to the specified position.
            void appear(twod target)
            {
                auto& screen = boss.base::area();
                auto  oldpos = screen.coor;
                auto  newpos = target - screen.size / 2;

                auto path = newpos - oldpos;
                auto time = skin::globals().switching;
                auto init = 0;
                auto func = constlinearAtoB<twod>(path, time, init);

                robo.pacify();
                robo.actify(func, [&](twod& x) { boss.base::moveby(x); boss.strike(); });
            }
            /*
            // pro::frame: Search for a non-overlapping form position in
            //             the visual tree along a specified direction.
            rect bounce(rect block, twod dir)
            {
                auto result = block.rotate(dir);
                auto parity = std::abs(dir.x) > std::abs(dir.y);

                for (auto xy : { parity, !parity })
                {
                    auto ray = result;
                    ray.coor[xy] += ray.size[xy];
                    ray.size[xy] = dir[xy] > 0 ? std::numeric_limits<int>::max()
                                               : std::numeric_limits<int>::min();

                    if (auto shadow = ray.trunc(boss.base::size))
                    {
                        auto direct = shadow.rotate(dir);
                        auto nearby = direct.coor[xy] + direct.size[xy];

                        foreach(boss.branch, boss.status.is.visible, [&](auto item)
                                {
                                    if (auto s = shadow.clip(item->square()))
                                    {
                                        auto next = dir[xy] > 0 ? s.coor[xy] : -(s.coor[xy] + s.size[xy]);
                                        if (next < nearby) nearby = next;
                                    }
                                });

                        result.size[xy] = (dir[xy] > 0 ? nearby : -nearby) - result.coor[xy];
                    }
                }

                return result;
            }
            */
            // pro::frame: Move the form no further than the parent canvas.
            void convey(twod delta, rect boundary)//, bool notify = true)
            {
                auto& r0 = boss.base::area();
                if (delta && r0.clip(boundary))
                {
                    auto r1 = r0;
                    auto r2 = boundary;
                    r1.coor -= r2.coor;

                    auto c = r1.rotate(-delta);
                    auto s = r2.size;
                    auto o = delta.less(dot_00, dot_00, dot_11);
                    if ((s + o).twod::inside(c.coor))
                    {
                        c.coor = std::clamp(c.coor + delta, dot_00, s);
                        auto new_coor = c.normalize().coor + r2.coor;
                        boss.moveto(new_coor);
                    }
                    else if (!r2.clip(r0))
                    {
                        boss.moveby(delta);
                    }
                }
            }
        };

        // pro: Form generator.
        class maker
            : public skill
        {
            using skill::boss,
                  skill::memo;

            cell mark;

            struct slot_t
            {
                rect slot{};
                twod step{};
                twod init{};
                bool ctrl{};
            };
            std::unordered_map<id_t, slot_t> slots;
            escx coder;

            void check_modifiers(hids& gear)
            {
                auto& data = slots[gear.id];
                auto state = !!gear.meta(hids::anyCtrl);
                if (data.ctrl != state)
                {
                    data.ctrl = state;
                    boss.deface(data.slot);
                }
            }
            void handle_init(hids& gear)
            {
                if (gear.capture(boss.bell::id))
                {
                    auto& data = slots[gear.id];
                    auto& slot = data.slot;
                    auto& init = data.init;
                    auto& step = data.step;

                    data.ctrl = gear.meta(hids::anyCtrl);
                    slot.coor = init = step = gear.coord;
                    slot.size = dot_00;
                    boss.deface(slot);
                    gear.dismiss();
                }
            }
            void handle_pull(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    check_modifiers(gear);
                    auto& data = slots[gear.id];
                    auto& slot = data.slot;
                    auto& init = data.init;
                    auto& step = data.step;

                    step += gear.delta.get();
                    slot.coor = std::min(init, step);
                    slot.size = std::max(std::abs(step - init), dot_00);
                    boss.deface(slot);
                    gear.dismiss();
                }
            }
            void handle_drop(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    slots.erase(gear.id);
                    gear.dismiss();
                    gear.setfree();
                }
            }
            void handle_stop(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    check_modifiers(gear);
                    auto& data = slots[gear.id];
                    if (data.slot)
                    {
                        gear.slot = data.slot;
                        gear.slot.coor += boss.base::coor();
                        gear.slot_forced = true;
                        boss.RISEUP(tier::request, e2::form::proceed::createby, gear);
                    }
                    slots.erase(gear.id);
                    gear.dismiss();
                    gear.setfree();
                }
            }

        public:
            maker(base&&) = delete;
            maker(base& boss)
                : skill{ boss },
                   mark{ skin::color(tone::selector) }
            {
                using drag = hids::events::mouse::button::drag;

                boss.LISTEN(tier::preview, hids::events::keybd::data::post, gear, memo)
                {
                    if (gear.captured(boss.bell::id)) check_modifiers(gear);
                };

                //todo unify - args... + template?
                //middle button
                //todo revise boss.LISTEN(tier::preview, drag::start::middle, gear, memo)
                boss.LISTEN(tier::release, drag::start::middle, gear, memo)
                {
                    handle_init(gear);
                };
                boss.LISTEN(tier::release, drag::pull::middle, gear, memo)
                {
                    handle_pull(gear);
                };
                boss.LISTEN(tier::release, drag::cancel::middle, gear, memo)
                {
                    handle_drop(gear);
                };
                boss.LISTEN(tier::release, drag::stop::middle, gear, memo)
                {
                    handle_stop(gear);
                };

                //todo unify
                //right button
                boss.LISTEN(tier::release, drag::start::right, gear, memo)
                {
                    handle_init(gear);
                };
                boss.LISTEN(tier::release, drag::pull::right, gear, memo)
                {
                    handle_pull(gear);
                };
                boss.LISTEN(tier::release, drag::cancel::right, gear, memo)
                {
                    handle_drop(gear);
                };
                boss.LISTEN(tier::release, drag::stop::right, gear, memo)
                {
                    handle_stop(gear);
                };

                boss.LISTEN(tier::general, hids::events::halt, gear, memo)
                {
                    handle_drop(gear);
                };

                boss.LISTEN(tier::release, /*!*/e2::postrender, canvas, memo)
                {
                    //todo Highlighted area drawn twice
                    for (auto const& [key, data] : slots)
                    {
                        auto slot = data.slot;
                        slot.coor -= canvas.coor();
                        if (auto area = canvas.area().clip<true>(slot))
                        {
                            if (data.ctrl)
                            {
                                area.coor -= dot_11;
                                area.size += dot_22;
                                auto mark = skin::color(tone::kb_focus);
                                auto fill = [&](cell& c) { c.fuse(mark); };
                                canvas.cage(area, dot_11, fill);
                                coder.wrp(wrap::off).add("capture area: ", slot);
                                //todo optimize para
                                auto caption = para(coder);
                                coder.clear();
                                auto header = *caption.lyric;
                                auto coor = area.coor + canvas.coor();
                                coor.y--;
                                header.move(coor);
                                canvas.fill(header, cell::shaders::contrast);
                            }
                            else
                            {
                                auto temp = canvas.view();
                                canvas.view(area);
                                canvas.fill(area, [&](cell& c) { c.fuse(mark); c.und(faux); });
                                canvas.blur(10);
                                coder.wrp(wrap::off).add(' ').add(slot.size.x).add(" × ").add(slot.size.y).add(' ');
                                //todo optimize para
                                auto caption = para(coder);
                                coder.clear();
                                auto header = *caption.lyric;
                                auto coor = area.coor + area.size + canvas.coor();
                                coor.x -= caption.length() - 1;
                                header.move(coor);
                                canvas.fill(header, cell::shaders::contrast);
                                canvas.view(temp);
                            }
                        }
                    }
                };
            }
        };

        // pro: Drag&drop functionality.
        class d_n_d
            : public skill
        {
            using skill::boss,
                  skill::memo;

            id_t under;
            bool drags;
            twod coord;
            wptr cover;

            void proceed(bool keep)
            {
                drags = faux;
                boss.SIGNAL(tier::anycast, e2::form::prop::lucidity, 0xFF); // Make target opaque.
                if (auto dest_ptr = cover.lock())
                {
                    auto& dest = *dest_ptr;
                    if (keep)
                    {
                        boss.SIGNAL(tier::preview, vtm::events::d_n_d::drop, what, ()); // Take core.
                        dest.SIGNAL(tier::release, vtm::events::d_n_d::drop, what); // Pass core.
                        boss.base::detach(); // The object kills itself.
                    }
                    else dest.SIGNAL(tier::release, vtm::events::d_n_d::abort, boss.This());
                }
                cover.reset();
                under = {};
            }

        public:
            d_n_d(base&&) = delete;
            d_n_d(base& boss)
                : skill{ boss },
                  drags{ faux },
                  under{      }
            {
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::start::any, gear, memo)
                {
                    if (boss.size().inside(gear.coord) && !gear.kbmod())
                    if (drags || !gear.capture(boss.id)) return;
                    {
                        drags = true;
                        coord = gear.coord;
                        under = {};
                    }
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::pull::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              coord = gear.coord - gear.delta.get();
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::stop::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              proceed(true);
                    gear.setfree();
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::cancel::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              proceed(true);
                    gear.setfree();
                };
                boss.LISTEN(tier::release, e2::render::prerender, parent_canvas, memo)
                {
                    if (!drags) return;
                    auto area = parent_canvas.core::area();
                    auto coor = coord - area.coor;
                    if (area.size.inside(coor))
                    {
                        auto& c = parent_canvas[coor];
                        auto new_under = c.link();
                        if (under != new_under)
                        {
                            auto object = vtm::events::d_n_d::ask.param();
                            if (auto old_object = bell::getref<base>(under))
                            {
                                old_object->RISEUP(tier::release, vtm::events::d_n_d::abort, object);
                            }
                            if (auto new_object = bell::getref<base>(new_under))
                            {
                                new_object->RISEUP(tier::release, vtm::events::d_n_d::ask, object);
                            }
                            boss.SIGNAL(tier::anycast, e2::form::prop::lucidity, object ? 0x80
                                                                                        : 0xFF); // Make it semi-transparent on success and opaque otherwise.
                            cover = object;
                            under = new_under;
                        }
                    }
                };
            }
        };
    }

    // vtm: User gate.
    struct gate
        : public ui::gate
    {
        pro::robot robot{*this }; // gate: Animation controller.
        pro::maker maker{*this }; // gate: Form generator.
        pro::align align{*this, nexthop }; // gate: Fullscreen access controller.
        pro::notes notes; // gate: Tooltips for user.

        gate(xipc uplink, view userid, si32 vtmode, xmls& config, si32 session_id)
            : ui::gate{ uplink, vtmode, config, userid, session_id, true },
              notes{*this, ansi::add(prompt::gate, props.title) }
        {
            //todo local=>nexthop
            local = faux;
            LISTEN(tier::release, e2::form::upon::vtree::attached, world_ptr)
            {
                nexthop = world_ptr;
            };
            LISTEN(tier::release, hids::events::keybd::data::post, gear, tokens)
            {
                if (gear)
                {
                    if (true /*...*/) //todo lookup taskbar kb shortcuts
                    {
                        //...
                        //pro::focus::set(applet, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        
                    }
                    else
                    {
                        //...
                    }
                }
            };
            LISTEN(tier::preview, hids::events::keybd::data::any, gear, tokens)
            {
                //todo deprecated
                //todo unify
                auto F12 = (gear.keycode == input::key::F12 && gear.meta(hids::anyAlt)) || gear.keystrokes == "\033[24;3~"s;
                if (F12 && gear.pressed) // Disconnect by Alt+F12.
                {
                    gear.owner.SIGNAL(tier::preview, e2::conio::quit, deal, ());
                    this->bell::expire<tier::preview>();
                    gear.set_handled(true);
                    return;
                }
                auto F10 = gear.keycode == input::key::F10 || gear.keystrokes == "\033[21~"s;
                if (F10 && gear.pressed)
                {
                    auto window_ptr = e2::form::layout::go::item.param();
                    this->RISEUP(tier::request, e2::form::layout::go::item, window_ptr); // Take current window.
                    if (!window_ptr)
                    {
                        this->SIGNAL(tier::general, e2::shutdown, msg, (utf::concat(prompt::gate, "Server shutdown")));
                        this->bell::expire<tier::preview>();
                        gear.set_handled(true);
                    }
                    return;
                }
                auto& keystrokes = gear.keystrokes;
                auto pgup = keystrokes == "\033[5;5~"s
                        || (keystrokes == "\033[5~"s && gear.meta(hids::anyCtrl));
                auto pgdn = keystrokes == "\033[6;5~"s
                        || (keystrokes == "\033[6~"s && gear.meta(hids::anyCtrl));
                if (pgup || pgdn)
                {
                    if (align.what.applet)
                    {
                        align.unbind();
                    }
                    auto window_ptr = e2::form::layout::go::item.param();
                    this->RISEUP(tier::request, e2::form::layout::go::item, window_ptr); // Take current window.
                    if (window_ptr) window_ptr->SIGNAL(tier::release, e2::form::layout::unselect, gear);

                    window_ptr.reset();
                    if (pgdn) this->RISEUP(tier::request, e2::form::layout::go::prev, window_ptr); // Take prev window.
                    else      this->RISEUP(tier::request, e2::form::layout::go::next, window_ptr); // Take next window.

                    if (window_ptr)
                    {
                        auto& window = *window_ptr;
                        window.SIGNAL(tier::release, e2::form::layout::selected, gear);
                        jump_to(window);
                        pro::focus::set(window_ptr, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                    }
                    //gear.dismiss();
                    this->bell::expire<tier::preview>(); //todo temp
                    gear.set_handled(true);
                }
            };
            LISTEN(tier::release, e2::form::layout::jumpto, window, tokens)
            {
                jump_to(window);
            };
            LISTEN(tier::release, hids::events::mouse::button::click::left, gear, tokens) // Go to another user's viewport.
            {
                if (gear.owner.id == this->id) return;
                auto center = this->base::coor() + gear.owner.base::size() / 2;
                gear.owner.SIGNAL(tier::release, e2::form::layout::shift, center);
            };
            //todo move it to the desk (dragging)
            mouse.draggable<hids::buttons::leftright>(true);
            mouse.draggable<hids::buttons::left>(true);
            LISTEN(tier::release, e2::form::drag::start::any, gear, tokens)
            {
                if (gear.owner.id != this->id) return;
                robot.pacify();
            };
            LISTEN(tier::release, e2::form::drag::pull::any, gear, tokens)
            {
                if (gear.owner.id != this->id) return;
                base::moveby(-gear.delta.get());
                base::deface();
            };
            LISTEN(tier::release, e2::form::drag::stop::any, gear, tokens)
            {
                if (gear.owner.id != this->id) return;
                robot.pacify();
                robot.actify(gear.fader<quadratic<twod>>(2s), [&](auto& x)
                {
                    base::moveby(-x);
                    base::deface();
                });
            };
            LISTEN(tier::release, e2::form::layout::shift, newpos, tokens)
            {
                this->SIGNAL(tier::request, e2::form::prop::viewport, viewport, ());
                move_viewport(newpos, viewport);
            };
            LISTEN(tier::release, /*!*/e2::render::any, canvas, tokens, (fullscreen_banner = page{ "Fullscreen Mode\n\n" }))
            {
                if (&canvas != &input.xmap) // Draw a shadow of user's terminal window for other users (spectators).
                {
                    auto gate_area = canvas.full();
                    if (canvas.cmode != svga::vt16 && canvas.cmode != svga::nt16) // Don't show shadow in poor color environment.
                    {
                        //todo revise
                        auto mark = skin::color(tone::shadow);
                        mark.bga(mark.bga() / 2);
                        canvas.fill(gate_area, [&](cell& c){ c.fuse(mark); });
                    }
                    auto saved_context = canvas.bump(dent{ 0,0,1,0 });
                    canvas.output(uname, dot_00, cell::shaders::contrast);
                    if (align.what.applet)
                    {
                        canvas.bump(dent{ -2,-2,-2,-1 });
                        canvas.cup(dot_00);
                        canvas.output(fullscreen_banner);
                        //todo revise
                        //canvas.output(title.head_page, cell::shaders::contrast);
                    }
                    canvas.bump(saved_context);
                }
            };
            LISTEN(tier::release, /*!*/e2::postrender, parent_canvas, tokens)
            {
                if (&parent_canvas != &input.xmap)
                {
                    if (uname.lyric) // Render foreign user names at their place.
                    {
                        draw_foreign_names(parent_canvas);
                    }
                    draw_mouse_pointer(parent_canvas);
                }
            };
        }

        void move_viewport(twod newpos, rect viewport)
        {
            auto oldpos = viewport.center();
            auto path = oldpos - newpos;
            auto time = skin::globals().switching;
            auto init = 0;
            auto func = constlinearAtoB<twod>(path, time, init);
            robot.pacify();
            robot.actify(func, [&](auto& x)
            {
                base::moveby(-x);
                base::strike();
            });
        }
        void jump_to(base& window)
        {
            SIGNAL(tier::request, e2::form::prop::viewport, viewport, ());
            auto object_area = window.area() + dent{ 2,2,1,1 };
            auto outside = viewport.unite(object_area);
            if (outside != viewport)
            {
                auto coor = outside.coor.equals(object_area.coor, object_area.coor, outside.coor + outside.size - viewport.size);
                auto center = viewport.center() + coor - viewport.coor;
                move_viewport(center, viewport);
            }
        }
        void rebuild_scene(auto& world, bool damaged)
        {
            if (damaged)
            {
                auto& canvas = input.xmap;
                canvas.wipe(world.id);
                if (align.what.applet)
                {
                    if (auto context = canvas.change_basis(base::area()))
                    {
                        align.what.applet->render(canvas);
                    }
                }
                else
                {
                    if (props.background_image.size())
                    {
                        //todo cache background
                        canvas.tile(props.background_image, cell::shaders::fuse);
                    }
                    world.redraw(canvas); // Put the rest of the world on my canvas.
                    if (applet) // Render main menu/application.
                    if (auto context = canvas.change_basis(base::area()))
                    {
                        //todo too hacky, unify
                        if (props.glow_fx) applet->render(canvas); // Render the taskbar menu twice to achieve the glow effect.
                                           applet->render(canvas);
                    }
                }
            }
            _rebuild_scene(damaged);
        }
    };

    // vtm: Desktopio Workspace.
    struct hall
        : public host
    {
    private:
        struct node // hall: Adapter for the object that going to be attached to the world.
        {
            bool highlighted = faux;
            si32 active = 0;
            tone color = { tone::brighter, tone::shadower };
            sptr object;
            zpos z_order = zpos::plain;
            subs tokens;

            node(sptr item)
                : object{ item }
            {
                auto& inst = *item;
                inst.LISTEN(tier::release, e2::form::prop::zorder, order, tokens)
                {
                    z_order = order;
                };
                inst.LISTEN(tier::release, e2::form::state::mouse, state, tokens)
                {
                    active = state;
                };
                inst.LISTEN(tier::release, e2::form::state::highlight, state, tokens)
                {
                    highlighted = state;
                };
                inst.LISTEN(tier::release, e2::form::state::color, new_color, tokens)
                {
                    color = new_color;
                };
                inst.SIGNAL(tier::request, e2::form::state::mouse, active);
                inst.SIGNAL(tier::request, e2::form::state::color, color);
            }
            // hall::node: Draw a navigation string.
            void fasten(face& canvas)
            {
                auto window = canvas.area();
                auto center = object->region.center();
                if (window.hittest(center) || object->hidden) return;
                auto origin = window.size / 2;
                center -= window.coor;
                //auto origin = twod{ 6, window.size.y - 3 };
                //header.usable = window.overlap(region);
                auto is_active = active || highlighted;
                auto& grade = skin::grade(is_active ? color.active
                                                    : color.passive);
                auto obj_id = object->id;
                auto pset = [&](twod p, byte k)
                {
                    //canvas[p].fuse(grade[k], obj_id, p - offset);
                    //canvas[p].fuse(grade[k], obj_id);
                    canvas[p].link(obj_id).bgc().mix_one(grade[k].bgc());
                };
                window.coor = dot_00;
                netxs::online(window, origin, center, pset);
            }
        };
        struct list // hall: List of objects that can be reordered, etc.
        {
            std::list<netxs::sptr<node>> items;

            template<class D>
            auto search(D head, D tail, id_t id)
            {
                if (items.size())
                {
                    auto test = [id](auto& a) { return a->object->id == id; };
                    return std::find_if(head, tail, test);
                }
                return tail;
            }

            operator bool () { return items.size(); }
            auto size()      { return items.size(); }
            auto back()      { return items.back()->object; }
            void append(sptr window_ptr)
            {
                auto& window = *window_ptr;
                window.LISTEN(tier::preview, e2::form::layout::expose, area, -)
                {
                    area = expose(window.id);
                    if (area) window.RISEUP(tier::release, e2::form::layout::expose, area);
                };
                window.LISTEN(tier::preview, e2::form::layout::bubble, area, -)
                {
                    area = bubble(window.id);
                    if (area) window.RISEUP(tier::release, e2::form::layout::bubble, area);
                };
                items.push_back(ptr::shared<node>(window_ptr));
            }
            //hall::list: Draw backpane for spectators.
            void prerender(face& canvas)
            {
                if (size() > 1)
                for (auto& item : items) item->fasten(canvas); // Draw strings.
                for (auto& item : items) item->object->render(canvas, true, true, faux); // Draw shadows without postrendering.
            }
            //hall::list: Draw windows.
            void render(face& canvas)
            {
                for (auto& item : items) item->fasten(canvas);
                for (auto& item : items) if (item->z_order == zpos::backmost) item->object->render(canvas);
                for (auto& item : items) if (item->z_order == zpos::plain   ) item->object->render(canvas);
                for (auto& item : items) if (item->z_order == zpos::topmost ) item->object->render(canvas);
            }
            //hall::list: Draw spectator's mouse pointers.
            void postrender(face& canvas)
            {
                for (auto& item : items) item->object->render(canvas, true, faux, true);
            }
            //hall::list: Delete all items.
            void reset()
            {
                items.clear();
            }
            rect remove(id_t id)
            {
                auto area = rect{};
                auto head = items.begin();
                auto tail = items.end();
                auto item = search(head, tail, id);
                if (item != tail)
                {
                    area = (**item).object->region;
                    items.erase(item);
                }
                return area;
            }
            rect bubble(id_t id)
            {
                auto head = items.rbegin();
                auto tail = items.rend();
                auto item = search(head, tail, id);

                if (item != head && item != tail)
                {
                    auto area = (**item).object->region;
                    if (!area.clip((**std::prev(item)).object->region))
                    {
                        auto shadow = *item;
                        items.erase(std::next(item).base());

                        while (--item != head
                            && !area.clip((**std::prev(item)).object->region))
                        { }

                        items.insert(item.base(), shadow);
                        return area;
                    }
                }

                return rect_00;
            }
            rect expose(id_t id)
            {
                auto head = items.rbegin();
                auto tail = items.rend();
                auto item = search(head, tail, id);

                if (item != head && item != tail)
                {
                    auto shadow = *item;
                    items.erase(std::next(item).base());
                    items.push_back(shadow);
                    if (shadow->object->hidden) // Restore if window minimized.
                    {
                        shadow->object->hidden = faux;
                    }
                    return shadow->object->region;
                }

                return rect_00;
            }
            auto rotate_next()
            {
                items.push_back(items.front());
                items.pop_front();
                return items.back();
            }
            auto rotate_prev()
            {
                items.push_front(items.back());
                items.pop_back();
                return items.back();
            }
        };
        struct depo // hall: Actors registry.
        {
            netxs::sptr<desk::apps> apps_ptr = ptr::shared(desk::apps{});
            netxs::sptr<desk::usrs> usrs_ptr = ptr::shared(desk::usrs{});
            netxs::sptr<desk::menu> menu_ptr = ptr::shared(desk::menu{});
            desk::apps& apps = *apps_ptr;
            desk::usrs& usrs = *usrs_ptr;
            desk::menu& menu = *menu_ptr;

            void append(sptr user)
            {
                usrs.push_back(user);
            }
            auto remove(sptr item_ptr)
            {
                auto found = faux;
                for (auto& [class_id, fxd_app_list] : apps) // Remove app.
                {
                    auto& [fixed, app_list] = fxd_app_list;
                    auto head = app_list.begin();
                    auto tail = app_list.end();
                    auto iter = std::find_if(head, tail, [&](auto& c) { return c == item_ptr; });
                    if (iter != tail)
                    {
                        app_list.erase(iter);
                        if (app_list.empty() && !fixed)
                        {
                            apps.erase(class_id);
                        }
                        found = true;
                        break;
                    }
                }
                { // Remove user.
                    auto head = usrs.begin();
                    auto tail = usrs.end();
                    auto iter = std::find_if(head, tail, [&](auto& c){ return c == item_ptr; });
                    if (iter != tail)
                    {
                        usrs.erase(iter);
                        found = true;
                    }
                }
                return found;
            }
            void reset()
            {
                apps.clear();
            }
        };

        using idls = std::vector<id_t>;
        using pool = netxs::generics::pool;

        list items; // hall: Child visual tree.
        list users; // hall: Scene spectators.
        depo dbase; // hall: Actors registry.
        twod vport; // hall: Last user's viewport position.
        pool async; // hall: Thread pool for parallel task execution.

        static auto window(link& what)
        {
            return ui::cake::ctor()
                ->plugin<pro::d_n_d>()
                ->plugin<pro::title>(what.header, what.footer)
                ->plugin<pro::notes>(what.header, dent{ 2,2,1,1 })
                ->plugin<pro::sizer>()
                ->plugin<pro::frame>()
                ->plugin<pro::light>()
                ->plugin<pro::focus>()
                ->limits(dot_11, { 400,200 }) //todo unify, set via config
                ->invoke([&](auto& boss)
                {
                    boss.base::kind(base::reflow_root);
                    boss.LISTEN(tier::preview, vtm::events::d_n_d::drop, what, -, (menuid = what.menuid))
                    {
                        if (auto applet = boss.pop_back())
                        {
                            boss.SIGNAL(tier::request, e2::form::prop::ui::header, what.header);
                            boss.SIGNAL(tier::request, e2::form::prop::ui::footer, what.footer);
                            what.applet = applet;
                            what.menuid = menuid;
                        }
                    };
                    boss.LISTEN(tier::release, e2::area, new_area)
                    {
                        boss.SIGNAL(tier::anycast, e2::form::upon::resized, new_area);
                    };
                    auto last_state = ptr::shared(faux);
                    boss.LISTEN(tier::release, e2::form::layout::selected, gear, -, (last_state))
                    {
                        *last_state = boss.hidden;
                        boss.hidden = faux; // Restore if it is hidden.
                    };
                    boss.LISTEN(tier::release, e2::form::layout::unselect, gear, -, (last_state))
                    {
                        if (*last_state == true) // Return to hidden state.
                        {
                            boss.hidden = true;
                        }
                    };
                    boss.LISTEN(tier::release, e2::form::layout::minimize, gear)
                    {
                        auto This = boss.This();
                        if (boss.hidden) // Restore if it is hidden.
                        {
                            boss.hidden = faux;
                            pro::focus::set(This, gear.id, gear.meta(hids::anyCtrl) ? pro::focus::solo::off
                                                                                    : pro::focus::solo::on, pro::focus::flip::off, true);
                        }
                        else // Hide if visible and refocus.
                        {
                            boss.hidden = true;
                            boss.RISEUP(tier::request, e2::form::state::keybd::find, gear_test, (gear.id, 0));
                            if (auto parent = boss.parent())
                            if (gear_test.second) // If it is focused pass the focus to the next desktop window.
                            {
                                parent->SIGNAL(tier::request, e2::form::state::keybd::next, gear_test, (gear.id, 0));
                                if (gear_test.second == 1) // If it is the last focused item.
                                {
                                    auto viewport = gear.owner.base::area();
                                    auto window = e2::form::layout::go::prev.param();
                                    auto hidden = true;
                                    do
                                    {
                                        parent->SIGNAL(tier::request, e2::form::layout::go::prev, window);
                                        if (window)
                                        {
                                            hidden = window->hidden;
                                        }
                                        else hidden = true;
                                    }
                                    while (window != This && (hidden == true || !viewport.hittest(window->center())));
                                    if (window != This)
                                    {
                                        pro::focus::set(window, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                                        This.reset();
                                    }
                                }
                                if (This) pro::focus::off(This, gear.id);
                            }
                        }
                    };
                    boss.LISTEN(tier::release, e2::form::prop::ui::header, title)
                    {
                        auto tooltip = " " + title + " ";
                        boss.SIGNAL(tier::preview, e2::form::prop::ui::tooltip, tooltip);
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::dblclick::left, gear)
                    {
                        boss.RISEUP(tier::release, e2::form::layout::fullscreen, gear);
                        gear.dismiss();
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear)
                    {
                        auto home = rect{ -dot_21, boss.base::size() + dot_21 * 2 }; // Including resizer grips.
                        if (!home.hittest(gear.coord))
                        {
                            auto center = boss.base::center();
                            gear.owner.SIGNAL(tier::release, e2::form::layout::shift, center);
                            boss.base::deface();
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::right, gear)
                    {
                        pro::focus::set(boss.This(), gear.id, pro::focus::solo::on, pro::focus::flip::off);
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::middle, gear)
                    {
                        pro::focus::set(boss.This(), gear.id, pro::focus::solo::on, pro::focus::flip::off);
                    };
                    boss.LISTEN(tier::release, e2::form::proceed::quit::any, fast)
                    {
                        boss.mouse.reset();
                        boss.base::detach(); // The object kills itself.
                    };
                    boss.LISTEN(tier::general, e2::conio::quit, deal) // Desktop shutdown.
                    {
                        boss.SIGNAL(tier::anycast, e2::form::proceed::quit::one, true); // Schedule a cleanup.
                    };
                    boss.LISTEN(tier::release, e2::dtor, p)
                    {
                        auto start = datetime::now();
                        boss.SIGNAL(tier::general, e2::cleanup, counter, ());
                        auto stop = datetime::now() - start;
                        if constexpr (debugmode) log(prompt::hall, "Garbage collection",
                                                    "\n\ttime ", utf::format(stop.count()), "ns",
                                                    "\n\tobjs ", counter.obj_count,
                                                    "\n\trefs ", counter.ref_count,
                                                    "\n\tdels ", counter.del_count);
                    };

                    auto what_copy = what;
                    what_copy.applet = {};
                    boss.LISTEN(tier::release, e2::form::layout::fullscreen, gear, -, (what_copy))
                    {
                        auto window_ptr = boss.This();
                        auto gear_id_list = pro::focus::get(window_ptr, true); // Expropriate all foci.
                        auto what = what_copy;
                        what.applet = window_ptr;
                        pro::focus::set(window_ptr, gear.id, pro::focus::solo::on, pro::focus::flip::off, true); // Refocus.
                        window_ptr->RISEUP(tier::request, e2::form::prop::ui::header, what.header);
                        window_ptr->RISEUP(tier::request, e2::form::prop::ui::footer, what.footer);
                        gear.owner.SIGNAL(tier::release, vtm::events::gate::fullscreen, what);
                    };
                });
        }
        auto create(link& what)
        {
            SIGNAL(tier::request, vtm::events::newapp, what);
            auto window_ptr = hall::window(what);
            auto& cfg = dbase.menu[what.menuid];
            if (cfg.winsize && !what.forced) window_ptr->extend({ what.square.coor, cfg.winsize });
            else                             window_ptr->extend(what.square);
            window_ptr->attach(what.applet);
            if constexpr (debugmode) log(prompt::hall, "App type: ", utf::debase(cfg.type), ", menu item id: ", utf::debase(what.menuid));
            this->branch(what.menuid, window_ptr, !cfg.hidden);
            window_ptr->SIGNAL(tier::anycast, e2::form::upon::started, this->This());
            return window_ptr;
        }

    protected:
        hall(xipc server, xmls& config, text defapp)
            : host{ server, config, pro::focus::mode::focusable }
        {
            auto current_module_file = os::process::binary();
            auto& apps_list = dbase.apps;
            auto& menu_list = dbase.menu;
            auto  free_list = std::list<std::pair<text, desk::spec>>{};
            auto  temp_list = free_list;
            auto  dflt_spec = desk::spec
            {
                .hidden   = faux,
                .slimmenu = faux,
                .type     = defapp,
            };
            auto find = [&](auto const& menuid) -> auto&
            {
                auto test = [&](auto& p) { return p.first == menuid; };

                auto iter_free = std::find_if(free_list.begin(), free_list.end(), test);
                if (iter_free != free_list.end()) return iter_free->second;

                auto iter_temp = std::find_if(temp_list.begin(), temp_list.end(), test);
                if (iter_temp != temp_list.end()) return iter_temp->second;

                return dflt_spec;
            };

            auto splitter_count = 0;
            for (auto item_ptr : host::config.list(path::item))
            {
                auto& item = *item_ptr;
                auto conf_rec = desk::spec{};
                //todo autogen id if absent
                conf_rec.splitter = item.take(attr::splitter, faux);
                conf_rec.menuid   = item.take(attr::id,       ""s );
                if (conf_rec.splitter)
                {
                    conf_rec.menuid = "splitter_" + std::to_string(splitter_count++);
                }
                else if (conf_rec.menuid.empty())
                {
                    log("%%Attribute '%attrid%' is missing, skip item", prompt::hall, utf::debase(attr::id));
                    continue;
                }
                auto label        = item.take(attr::label, ""s);
                conf_rec.label    = label.empty() ? conf_rec.menuid : label;
                conf_rec.alias    = item.take(attr::alias, ""s);
                auto& fallback = conf_rec.alias.empty() ? dflt_spec
                                                        : find(conf_rec.alias);
                conf_rec.hidden   = item.take(attr::hidden,   fallback.hidden  );
                conf_rec.notes    = item.take(attr::notes,    fallback.notes   );
                conf_rec.title    = item.take(attr::title,    fallback.title   );
                conf_rec.footer   = item.take(attr::footer,   fallback.footer  );
                conf_rec.bgc      = item.take(attr::bgc,      fallback.bgc     );
                conf_rec.fgc      = item.take(attr::fgc,      fallback.fgc     );
                conf_rec.winsize  = item.take(attr::winsize,  fallback.winsize );
                conf_rec.wincoor  = item.take(attr::wincoor,  fallback.wincoor );
                conf_rec.slimmenu = item.take(attr::slimmenu, fallback.slimmenu);
                conf_rec.hotkey   = item.take(attr::hotkey,   fallback.hotkey  ); //todo register hotkey
                conf_rec.cwd      = item.take(attr::cwd,      fallback.cwd     );
                conf_rec.param    = item.take(attr::param,    fallback.param   );
                conf_rec.type     = item.take(attr::type,     fallback.type    );
                auto patch        = item.list(attr::config);
                if (patch.size()) conf_rec.patch = patch.front()->snapshot();
                if (conf_rec.title.empty()) conf_rec.title = conf_rec.menuid + (conf_rec.param.empty() ? ""s : ": " + conf_rec.param);

                utf::to_low(conf_rec.type);
                utf::change(conf_rec.title,  "$0", current_module_file);
                utf::change(conf_rec.footer, "$0", current_module_file);
                utf::change(conf_rec.label,  "$0", current_module_file);
                utf::change(conf_rec.notes,  "$0", current_module_file);
                utf::change(conf_rec.param,  "$0", current_module_file);

                if (conf_rec.hidden) temp_list.emplace_back(std::move(conf_rec.menuid), std::move(conf_rec));
                else                 free_list.emplace_back(std::move(conf_rec.menuid), std::move(conf_rec));
            }
            for (auto& [menuid, conf_rec] : free_list)
            {
                apps_list[menuid];
                menu_list.emplace(std::move(menuid), std::move(conf_rec));
            }
            for (auto& [menuid, conf_rec] : temp_list)
            {
                menu_list.emplace(std::move(menuid), std::move(conf_rec));
            }

            LISTEN(tier::general, e2::timer::any, timestamp, tokens)
            {
                auto damaged = !host::debris.empty();
                host::debris.clear();
                for (auto& u : users.items)
                {
                    if (auto client = std::dynamic_pointer_cast<gate>(u->object))
                    {
                        client->rebuild_scene(*this, damaged);
                    }
                }
            };
            LISTEN(tier::release, vtm::events::gate::restore, what)
            {
                auto& cfg = dbase.menu[what.menuid];
                branch(what.menuid, what.applet, !cfg.hidden);
            };
            LISTEN(tier::request, vtm::events::apptype, what)
            {
                auto& setup = dbase.menu[what.menuid];
                what.kindid = setup.type;
            };
            LISTEN(tier::request, vtm::events::newapp, what)
            {
                auto& setup = dbase.menu[what.menuid];
                auto& maker = app::shared::builder(setup.type);
                what.applet = maker(setup.cwd, setup.param, host::config, setup.patch);
                what.header = setup.title;
                what.footer = setup.footer;
                if (setup.bgc     ) what.applet->SIGNAL(tier::anycast, e2::form::prop::colors::bg,   setup.bgc);
                if (setup.fgc     ) what.applet->SIGNAL(tier::anycast, e2::form::prop::colors::fg,   setup.fgc);
                if (setup.slimmenu) what.applet->SIGNAL(tier::anycast, e2::form::prop::ui::slimmenu, setup.slimmenu);
            };
            LISTEN(tier::general, e2::conio::logs, utf8) // Forward logs from brokers.
            {
                log<faux>(utf8);
            };
            LISTEN(tier::general, e2::form::global::lucidity, alpha)
            {
                if (alpha == -1)
                {
                    alpha = skin::globals().lucidity;
                }
                else
                {
                    alpha = std::clamp(alpha, 0, 255);
                    skin::globals().lucidity = alpha;
                    this->SIGNAL(tier::preview, e2::form::global::lucidity, alpha);
                }
            };
            LISTEN(tier::release, e2::form::layout::bubble, area)
            {
                //auto region = items.bubble(inst.bell::id);
                host::denote(area);
            };
            LISTEN(tier::release, e2::form::layout::expose, area)
            {
                //auto area = items.expose(inst.bell::id);
                host::denote(area);
            };
            LISTEN(tier::request, desk::events::usrs, usrs_ptr)
            {
                usrs_ptr = dbase.usrs_ptr;
            };
            LISTEN(tier::request, desk::events::apps, apps_ptr)
            {
                apps_ptr = dbase.apps_ptr;
            };
            LISTEN(tier::request, desk::events::menu, menu_ptr)
            {
                menu_ptr = dbase.menu_ptr;
            };
            //todo unify
            LISTEN(tier::request, e2::form::layout::go::next, next)
            {
                if (items)
                if (auto next_ptr = items.rotate_next())
                {
                    next = next_ptr->object;
                }
            };
            LISTEN(tier::request, e2::form::layout::go::prev, prev)
            {
                if (items)
                if (auto prev_ptr = items.rotate_prev())
                {
                    prev = prev_ptr->object;
                }
            };
            LISTEN(tier::request, e2::form::layout::go::item, item)
            {
                if (items) item = items.back();
            };
            LISTEN(tier::request, e2::form::proceed::createby, gear)
            {
                auto& gate = gear.owner;
                auto location = gear.slot;
                if (gear.meta(hids::anyCtrl))
                {
                    log(prompt::hall, "Area copied to clipboard ", location);
                    gate.SIGNAL(tier::release, e2::command::printscreen, gear);
                }
                else
                {
                    auto what = link{ .square = gear.slot, .forced = gear.slot_forced };
                    gate.SIGNAL(tier::request, e2::data::changed, what.menuid);
                    if (auto window = create(what))
                    {
                        //window->LISTEN(tier::release, e2::form::upon::vtree::detached, master)
                        //{
                        //    log(prompt::hall, "Objects count: ", items.size());
                        //};
                        pro::focus::set(window, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        window->SIGNAL(tier::anycast, e2::form::upon::created, gear); // Tile should change the menu item.
                    }
                }
            };
            LISTEN(tier::request, vtm::events::handoff, what)
            {
                auto& cfg = dbase.menu[what.menuid];
                auto slot = window(what);
                slot->extend(what.square);
                slot->attach(what.applet);
                log("%%Attach type=%itemtype% menuid=%id%", prompt::hall, utf::debase(cfg.type), utf::debase(what.menuid));
                this->branch(what.menuid, slot, !cfg.hidden);
                slot->SIGNAL(tier::anycast, e2::form::upon::started, this->This());
                what.applet = slot;
            };
            LISTEN(tier::release, hids::events::keybd::data::any, gear) // Last resort for unhandled kb event.
            {
                if (gear)
                {
                    gear.owner.SIGNAL(tier::release, hids::events::keybd::data::post, gear);
                }
            };
            LISTEN(tier::preview, hids::events::keybd::focus::cut, seed)
            {
                //todo revise: dtvt-app focus state can be wrong when user reconnects
                //if (seed.id == id_t{})
                //{
                //    this->SIGNAL(tier::release, hids::events::keybd::focus::bus::off, seed);
                //}
                //else
                if (auto gear_ptr = bell::getref<hids>(seed.id))
                {
                    auto& gear = *gear_ptr;
                    //seed.item = this->This();
                    gear.owner.SIGNAL(tier::preview, hids::events::keybd::focus::cut, seed);
                }
            };
            LISTEN(tier::preview, hids::events::keybd::focus::set, seed)
            {
                //todo revise
                //if (seed.id == id_t{})
                //{
                //    this->SIGNAL(tier::release, hids::events::keybd::focus::bus::on, seed);
                //}
                //else
                if (auto gear_ptr = bell::getref<hids>(seed.id))
                {
                    auto& gear = *gear_ptr;
                    seed.item = this->This();
                    gear.owner.SIGNAL(tier::preview, hids::events::keybd::focus::set, seed);
                }
            };
        }

    public:
        // hall: Autorun apps from config.
        void autorun()
        {
            vport = host::config.take(path::viewport, dot_00);
            auto what = link{};
            auto apps = host::config.list(path::autorun);
            auto foci = book{};
            foci.reserve(apps.size());
            for (auto app_ptr : apps)
            {
                auto& app = *app_ptr;
                if (!app.fake)
                {
                    what.menuid =   app.take(attr::id, ""s);
                    what.square = { app.take(attr::wincoor, dot_00),
                                    app.take(attr::winsize, twod{ 80,25 }) };
                    auto focused =  app.take(attr::focused, faux);
                    what.forced = !!what.square.size;
                    if (what.menuid.size())
                    {
                        auto window_ptr = create(what);
                        if (focused) foci.push_back(window_ptr);
                    }
                    else log(prompt::hall, "Unexpected empty app id in autorun configuration");
                }
            }
            auto count = 0;
            for (auto& window_ptr : foci)
            {
                pro::focus::set(window_ptr, id_t{}, count++ ? pro::focus::solo::off // Reset all foci on the first item.
                                                            : pro::focus::solo::on, pro::focus::flip::off);
            }
            if constexpr (debugmode)
            {
                SIGNAL(tier::request, e2::form::state::keybd::next, gear_test, (0,0));
                if (gear_test.second) log(prompt::hall, "Autofocused items count: ", gear_test.second);
            }
        }
        // hall: .
        void redraw(face& canvas)
        {
            users.prerender (canvas); // Draw backpane for spectators.
            items.render    (canvas); // Draw objects of the world.
            users.postrender(canvas); // Draw spectator's mouse pointers.
        }
        // hall: .
        template<class P>
        void run(P process)
        {
            async.run(process);
        }
        // hall: Attach a new item to the scene.
        void branch(text const& menuid, sptr item, bool fixed = true)
        {
            if (!host::active) return;
            items.append(item);
            item->base::root(true);
            auto& [stat, list] = dbase.apps[menuid];
            stat = fixed;
            list.push_back(item);
            item->SIGNAL(tier::release, e2::form::upon::vtree::attached, base::This());
            item->SIGNAL(tier::anycast, vtm::events::attached, base::This());
            this->SIGNAL(tier::release, desk::events::apps, dbase.apps_ptr);
        }
        // hall: Create a new user gate.
        auto invite(xipc client, view userid, si32 vtmode, twod winsz, xmls config, si32 session_id)
        {
            auto lock = netxs::events::unique_lock();
            auto user = base::create<gate>(client, userid, vtmode, config, session_id);
            users.append(user);
            dbase.append(user);
            user->SIGNAL(tier::release, e2::form::upon::vtree::attached, base::This());
            this->SIGNAL(tier::release, desk::events::usrs, dbase.usrs_ptr);
            user->LISTEN(tier::release, e2::conio::winsz, new_size, -)
            {
                user->rebuild_scene(*this, true);
            };
            //todo make it configurable
            auto patch = ""s;
            auto deskmenu = app::shared::builder(app::desk::id)("", utf::concat(user->id, ";", user->props.os_user_id, ";", user->props.selected), config, patch);
            user->attach(deskmenu);
            user->base::resize(winsz);
            if (vport) user->base::moveto(vport); // Restore user's last position.
            lock.unlock();
            user->launch();
        }
        // hall: Detach user/window.
        void remove(sptr item_ptr) override
        {
            auto& inst = *item_ptr;
            host::denote(items.remove(inst.id));
            auto block = users.remove(inst.id);
            if (block) // Save user's viewport last position.
            {
                host::denote(block);
                vport = block.coor;
            }
            if (dbase.remove(item_ptr))
            {
                inst.SIGNAL(tier::release, e2::form::upon::vtree::detached, This());
            }
            if (items.size()) // Pass focus to the top most object.
            {
                auto last_ptr = items.back();
                item_ptr->RISEUP(tier::request, e2::form::state::keybd::enlist, gear_id_list, ());
                for (auto gear_id : gear_id_list)
                {
                    if (auto gear_ptr = bell::getref<hids>(gear_id))
                    {
                        SIGNAL(tier::request, e2::form::state::keybd::next, gear_test, (gear_id, 0));
                        if (gear_test.second == 1) // If it is the last focused item.
                        {
                            pro::focus::set(last_ptr, gear_id, pro::focus::solo::off, pro::focus::flip::off);
                        }
                    }
                }
            }
            SIGNAL(tier::release, desk::events::apps, dbase.apps_ptr); // Update taskbar app list.
        }
        // hall: Shutdown.
        void stop()
        {
            log(prompt::hall, "Server shutdown");
            SIGNAL(tier::general, e2::conio::quit, deal, ()); // Trigger to disconnect all users and monitors.
            async.stop(); // Wait until all users and monitors are disconnected.
            if constexpr (debugmode) log(prompt::hall, "Session control stopped");
            netxs::events::dequeue(); // Wait until all cleanups are completed.
            host::quartz.stop();
            auto lock = netxs::events::sync{};
            host::mouse.reset(); // Release the captured mouse.
            host::tokens.reset();
            dbase.reset();
            items.reset();
        }
    };
}