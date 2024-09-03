#include "object_factory.hh"
#include "absorber.hh"
#include "anchor.hh"
#include "angulardamper.hh"
#include "angularvelmeter.hh"
#include "animal.hh"
#include "backpack.hh"
#include "ball.hh"
#include "battery.hh"
#include "beam.hh"
#include "bomber.hh"
#include "boundary.hh"
#include "box.hh"
#include "breadboard.hh"
#include "button.hh"
#include "cable.hh"
#include "camera_rotator.hh"
#include "cavg.hh"
#include "ceilgate.hh"
#include "checkpoint.hh"
#include "clamp.hh"
#include "clip.hh"
#include "command.hh"
#include "companion.hh"
#include "conveyor.hh"
#include "corner.hh"
#include "crane.hh"
#include "ctrlbase.hh"
#include "cup.hh"
#include "cursorfield.hh"
#include "cylinder.hh"
#include "damper.hh"
#include "decay.hh"
#include "decorations.hh"
#include "display.hh"
#include "dragfield.hh"
#include "eatan2.hh"
#include "egraph.hh"
#include "elimit.hh"
#include "emitter.hh"
#include "epsilon.hh"
#include "erandom.hh"
#include "escript.hh"
#include "eventlistener.hh"
#include "explosive.hh"
#include "factory.hh"
#include "fan.hh"
#include "fifo.hh"
#include "floorgate.hh"
#include "fluid.hh"
#include "fxemitter.hh"
#include "gameman.hh"
#include "gear.hh"
#include "gearbox.hh"
#include "generator.hh"
#include "goal.hh"
#include "gravityman.hh"
#include "group.hh"
#include "gyroscope.hh"
#include "i2o1gate.hh"
#include "impact_sensor.hh"
#include "invertergate.hh"
#include "iomiscgate.hh"
#include "item.hh"
#include "jumper.hh"
#include "key_listener.hh"
#include "ladder.hh"
#include "levelman.hh"
#include "linear_decay.hh"
#include "lmotor.hh"
#include "lobber.hh"
#include "magconn.hh"
#include "magnet.hh"
#include "mavg.hh"
#include "mini_transmitter.hh"
#include "minibot.hh"
#include "motor.hh"
#include "object_finder.hh"
#include "objectfield.hh"
#include "oilrig.hh"
#include "panel.hh"
#include "pipeline.hh"
#include "pivot.hh"
#include "pixel.hh"
#include "pkgwarp.hh"
#include "plant.hh"
#include "pointer.hh"
#include "polygon.hh"
#include "prompt.hh"
#include "proximitysensor.hh"
#include "ragdoll.hh"
#include "rail.hh"
#include "rc_activator.hh"
#include "receiver.hh"
#include "repair_station.hh"
#include "resistor.hh"
#include "resource.hh"
#include "robot.hh"
#include "robotman.hh"
#include "rocket.hh"
#include "rope.hh"
#include "rubberband.hh"
#include "sawtooth.hh"
#include "scanner.hh"
#include "screenshot_marker.hh"
#include "scup.hh"
#include "seesaw.hh"
#include "sequencer.hh"
#include "sfxemitter.hh"
#include "shelf.hh"
#include "sincos.hh"
#include "sinewave.hh"
#include "snapgate.hh"
#include "soundman.hh"
#include "sparsifier.hh"
#include "sparsifier_plus.hh"
#include "speaker.hh"
#include "spikebot.hh"
#include "spikes.hh"
#include "splank.hh"
#include "sqrtgate.hh"
#include "squaregate.hh"
#include "stabilizer.hh"
#include "statesaver.hh"
#include "sticky.hh"
#include "switch.hh"
#include "tester.hh"
#include "tiltmeter.hh"
#include "timectrl.hh"
#include "timer.hh"
#include "toggler.hh"
#include "tpixel.hh"
#include "trampoline.hh"
#include "transmitter.hh"
#include "treasure_chest.hh"
#include "valueshift.hh"
#include "var_getter.hh"
#include "var_setter.hh"
#include "velmeter.hh"
#include "vendor.hh"
#include "weight.hh"
#include "wheel.hh"
#include "wmotor.hh"
#include "world.hh"
#include "ysplitter.hh"
#include "muladd.hh"
#include "esub.hh"
#include "player_activator.hh"

static entity* new_plank(void){return new beam(BEAM_THICK);};
static entity* new_thinplank(void){return new beam(BEAM_THIN);};
static entity* new_robot(void){return new robot();};
static entity* new_shelf(void){return new shelf();};
static entity* new_xorgate(void){return new xorgate();};
static entity* new_orgate(void){return new orgate();};

static entity* new_andgate(void){return new andgate();};
static entity* new_invertergate(void){return new invertergate();};
static entity* new_integergate(void){return new integergate();};
static entity* new_ceilgate(void){return new ceilgate();};
static entity* new_squaregate(void){return new squaregate();};
static entity* new_sqrtgate(void){return new sqrtgate();};
static entity* new_ball_wood(void){return new ball(0);};
static entity* new_ball_iron(void){return new ball(1);};
static entity* new_generator(void){return new generator();};
static entity* new_powercable(void){return new cable(CABLE_BLACK);};
static entity* new_signalcable(void){return new cable(CABLE_RED);};
static entity* new_ifacecable(void){return new cable(CABLE_BLUE);};
static entity* new_wmotor(void){return new wmotor();};
static entity* new_gear(void){return new gear();};
static entity* new_trampoline(void){return new trampoline();};
static entity* new_debugger(void){return new tester();};
static entity* new_button(void){return new button(0);};
static entity* new_rope(void){return new rope();};
static entity* new_sticky(void)
{
    sticky *s = new sticky();
    if (s->get_slot() == -1) {
        delete s;
        return 0;
    } else {
        return static_cast<entity*>(s);
    }
};
static entity* new_breadboard(void){return new breadboard();};
static entity* new_nandgate(void){return new nandgate();};
static entity* new_motor(void){return new motor(MOTOR_TYPE_DEFAULT);};
static entity* new_wheel(void){return new wheel();};
static entity* new_cup(void){return new cup();};
static entity* new_cylinder(void){return new cylinder(0);};
static entity* new_goal(void){return new goal();};
static entity* new_command(void){return new command();};
static entity* new_smallpanel(void){return new panel(PANEL_SMALL);};
static entity* new_xsmallpanel(void){return new panel(PANEL_XSMALL);};
static entity* new_mpanel(void){return new panel(PANEL_MEDIUM);};
static entity* new_bigpanel(void){return new panel(PANEL_BIG);};
static entity* new_sparsifier(void){return new sparsifier();};
static entity* new_besserwisser(void){return new besserwisser();};
static entity* new_magnet(void){return new magnet(0);};
static entity* new_electromagnet(void){return new magnet(1);};
static entity* new_railstraight(void){return new rail(RAIL_STRAIGHT);};
static entity* new_railskewed(void){return new rail(RAIL_SKEWED);};
static entity* new_railskewed2(void){return new rail(RAIL_SKEWED2);};
static entity* new_rail45deg(void){return new rail(RAIL_45DEG);};
static entity* new_servomotor(void){return new motor(MOTOR_TYPE_SERVO);};
static entity* new_simplemotor(void){return new motor(MOTOR_TYPE_SIMPLE);};
static entity* new_gyroscope(void){return new gyroscope();};
static entity* new_lmotor(void){return new lmotor(false);};
static entity* new_lsmotor(void){return new lmotor(true);};
static entity* new_magplug(void){return new magplug();};
static entity* new_magsock(void){return new magsock();};
static entity* new_switch(void){return new switcher();};
static entity* new_fifo(void){return new fifo();};
static entity* new_cavg(void){return new cavg();};
static entity* new_epsilon(void){return new epsilon();};
static entity* new_mavg(void){return new mavg();};
static entity* new_seesaw(void){return new seesaw();};
static entity* new_pivot(void){return new pivot_1();};
static entity* new_pivot2(void){return new pivot_2();};
static entity* new_ragdoll(void){return new ragdoll();};
static entity* new_clamp(void){return new clamp();};
static entity* new_toggler(void){return new toggler();};
static entity* new_pipeline(void){return new pipeline();};
static entity* new_splank(void){return new splank();};
static entity* new_interfaceclip(void){return new clip(CLIP_INTERFACE);};
static entity* new_signalclip(void){return new clip(CLIP_SIGNAL);};
static entity* new_gearbox(void){return new gearbox();};
static entity* new_ctrlmini(void){return new ctrlmini();};
static entity* new_ctrlservo(void){return new ctrlservo();};
static entity* new_ctrlfplus(void){return new ctrlfplus();};
static entity* new_battery3v(void){return new battery(BATTERY_3V);};
static entity* new_damper1(void){return new damper_1();};
static entity* new_damper2(void){return new damper_2();};
static entity* new_gameman(void){return new gameman();};
static entity* new_robotman(void){return new robotman();};
static entity* new_chunk(void){return 0;};
static entity* new_rubberbeam(void){return new beam(BEAM_RUBBER);};
static entity* new_corner(void){return new corner();};
static entity* new_scanner(void){return new scanner();};
static entity* new_tiltmeter(void){return new tiltmeter();};
static entity* new_thruster(void){return new rocket(0);};
static entity* new_rocket(void){return new rocket(1);};
static entity* new_proximitysensor(void){return new proximitysensor();};
static entity* new_ysplitter(void){return new ysplitter();};
static entity* new_valueshift(void){return new valueshift();};
static entity* new_togglebutton(void){return new button(1);};
static entity* new_ifgate(void){return new ifgate();};
static entity* new_idfield(void){return new objectfield(OBJECT_FIELD_ID);};
static entity* new_objectfield(void){return new objectfield(OBJECT_FIELD_OBJECT);};
static entity* new_target_setter(void){return new objectfield(OBJECT_FIELD_TARGET_SETTER);};
static entity* new_interactive_cylinder(void){return new cylinder(1);};
static entity* new_dragfield(void){return new dragfield();};
static entity* new_emitter(void){return new emitter(1);};
static entity* new_miniemitter(void){return new emitter(0);};
static entity* new_landmine(void){return new explosive(EXPLOSIVE_LANDMINE);};
static entity* new_bomb(void){return new explosive(EXPLOSIVE_BOMB);};
static entity* new_absorber(void){return new absorber(1);};
static entity* new_miniabsorber(void){return new absorber(0);};
static entity* new_timer(void){return new timer();};
static entity* new_conveyor(void){return new conveyor();};
static entity* new_plasticbeam(void){return new beam(BEAM_PLASTIC);};
static entity* new_woodbox(void){return new box(0);};
static entity* new_resistor(void){return new resistor();};
static entity* new_rubberband1(void){return new rubberband_1();};
static entity* new_rubberband2(void){return new rubberband_2();};
static entity* new_angulardamper(void){return new angulardamper();};
static entity* new_object_finder(void){return new object_finder();};
static entity* new_sincos(void){return new esincos();};
static entity* new_sinewave(void){return new sinewave();};
static entity* new_erandom(void){return new erandom();};
static entity* new_memory(void){return new memory();};
static entity* new_impact_sensor(void){return new impact_sensor(false);};
static entity* new_pressure_sensor(void){return new impact_sensor(true);};
static entity* new_gravity_manager(void){return new gravityman(GRAVITY_MANAGER);};
static entity* new_gravity_setter(void){return new gravityman(GRAVITY_SETTER);};
static entity* new_sawtooth(void){return new sawtooth();};
static entity* new_jumper(void){return new jumper();};
static entity* new_ibox(void){return new box(1);};
static entity* new_pbox(void){return new box(2);};
static entity* new_iball(void){return new ball(2);};
static entity* new_halfunpack(void){return new halfunpack();};
static entity* new_halfpack(void){return new halfpack();};
static entity* new_eatan2(void){return new eatan2();};
static entity* new_pointer(void){return new pointer();};
static entity* new_spikes(void){return new spikes();};
static entity* new_sum(void){return new sum();};
static entity* new_avg(void){return new avg();};
static entity* new_muladd(void){return new muladd();};
static entity* new_screenshot_marker(void){return new screenshot_marker();};
static entity* new_pixel(void){return new pixel();};
static entity* new_tpixel(void){return new tpixel();};
static entity* new_transmitter(void){return new transmitter(0);};
static entity* new_receiver(void) { return new receiver(); };
static entity* new_broadcaster(void){return new transmitter(1);};
static entity* new_fan(void){return new fan();};
static entity* new_min(void){return new emin();};
static entity* new_max(void){return new emax();};
static entity* new_backpack(void){return new backpack();};
static entity* new_stabilizer(void){return new estabilizer();};
static entity* new_pkgwarp(void){return new pkgwarp();};
static entity* new_pkgstatus(void){return new pkgstatus();};
static entity* new_camtargeter(void){return new camtargeter();};
static entity* new_condenser(void){return new condenser();};
static entity* new_fxemitter(void){return new fxemitter();};
static entity* new_emul(void){return new emul();};
static entity* new_esub(void){return new esub();};
static entity* new_minitransmitter(void){return new mini_transmitter();};
static entity* new_checkpoint(void){return new checkpoint();};
static entity* new_spikebot(void){return new spikebot();};
static entity* new_mini_spikebot(void){return new mini_spikebot();};
static entity* new_minibot(void){return new minibot();};
static entity* new_companion(void){return new companion();};
static entity* new_bomber(void){return new bomber();};
static entity* new_lobber(void){return new lobber();};
static entity* new_hpcontrol(void){return new hp_control();};
static entity* new_laser(void){entity *e = new scanner(); e->set_property(0, 1.f); return e;};
static entity* new_multiemitter(void){return new emitter(2);};
static entity* new_angularvelmeter(void){return new angularvelmeter();};
static entity* new_mirror(void){return new mirror();};
static entity* new_lasersensor(void){return new laser_sensor();};
static entity* new_velmeter(void){return new velmeter();};
static entity* new_wrapadd(void){return new wrapadd();};
static entity* new_wrapsub(void){return new wrapsub();};
static entity* new_wrapdist(void){return new ewrapdist();};
static entity* new_eventlistener(void){return new eventlistener();};
static entity* new_passive_display(void){return new passive_display();};
static entity* new_rcactivator(void){return new rcactivator();};
static entity* new_cursor_finder(void){return new cursor_finder();};
static entity* new_autoabsorber(void){return new autoabsorber();};
static entity* new_weight(void){return new weight();};
static entity* new_decay(void){return new decay();};
static entity* new_zoomer(void){return new zoomer();};
static entity* new_timectrl(void){return new timectrl();};
static entity* new_prompt(void){return new prompt();};
static entity* new_graph(void){return new egraph();};
static entity* new_wrapcondenser(void){return new wrapcondenser();};
static entity* new_ifelse(void){return new ifelse();};
static entity* new_sfxemitter(void)
{
    if (W->level.version < LEVEL_VERSION_1_5_1) {
        return new sfxemitter();
    } else {
        return new sfxemitter_2();
    }
};
static entity* new_cmpe(void){return new cmpe();};
static entity* new_cmpl(void){return new cmpl();};
static entity* new_cmple(void){return new cmple();};
static entity* new_speaker(void){return new speaker();};
static entity* new_snap(void){return new snapgate();};
static entity* new_var_getter(void){return new var_getter();};
static entity* new_var_setter(void){return new var_setter();};
static entity* new_sequencer(void){return new sequencer();};
static entity* new_ghost(void){return new ghost();};
static entity* new_cursorfield(void){return new cursorfield();};
static entity* new_escript(void){return new escript();};
static entity* new_ldecay(void){return new ldecay();};
static entity* new_elimit(void){return new elimit();};
static entity* new_item(void){return new item();};
static entity* new_oilrig(void){return new oilrig();};
static entity* new_factory(void){return new factory(FACTORY_GENERIC);};
static entity* new_crane(void){return new crane();};
static entity* new_fluid(void){return new fluid();};
static entity* new_localgravity(void){return new localgravity();};
static entity* new_autoprotector(void){return new autoprotector();};
static entity* new_active_display(void){return new active_display();};
static entity* new_boundary(void){return new boundary();};
static entity* new_suction_cup(void){return new scup();};
static entity* new_robot_factory(void){return new factory(FACTORY_ROBOT);};
static entity* new_armory(void){return new factory(FACTORY_ARMORY);};
static entity* new_separator(void){return new beam(BEAM_SEP);};
static entity* new_room(void){return new room();};
static entity* new_oil_mixer(void){return new factory(FACTORY_OIL_MIXER);};
static entity* new_repair_station(void){return new repair_station();};
static entity* new_guardpoint(void){return new anchor(ANCHOR_GUARDPOINT);};
static entity* new_ladder(void){return new ladder();};
static entity* new_resource(void){return new resource();};
static entity* new_plant(void){return new plant();};
static entity* new_vendor(void){return new vendor();};
static entity* new_animal(void){return new animal(ANIMAL_TYPE_COW);};
static entity* new_soundman(void){return new soundman();};
static entity* new_player_activator(void){return new player_activator();};
static entity* new_plastic_polygon(void){return new polygon(MATERIAL_PLASTIC);};
static entity* new_key_listener(void){return new key_listener();};
static entity* new_statesaver(void){return new statesaver();};
static entity* new_ifselect(void){return new ifselect();};
static entity* new_camera_rotator(void){return new camera_rotator();};
static entity* new_level_manager(void){return new levelman();};
static entity* new_treasure_chest(void){return new treasure_chest();};
static entity* new_decoration(void){return new decoration();};
static entity* new_megasplitter(void){return new megasplitter();};
static entity* new_ladder_step(void){return new ladder_step();};

uint32_t of::_id = 1;

static entity* (*c_creator[])(void) = {
    &new_thinplank, /* 0 */
    &new_plank,
    &new_ball_wood,
    &new_cylinder,
    &new_splank,
    &new_shelf, /* 5 */
    &new_ball_iron,
    &new_railstraight,
    &new_railskewed,
    &new_railskewed2,
    &new_rail45deg, /* 10 */
    &new_trampoline,
    &new_rope,
    &new_wheel,
    &new_magnet,
    &new_seesaw, /* 15 */
    &new_pivot,
    &new_pipeline,
    &new_gearbox,
    &new_damper1,
    &new_motor, /* 20 */
    &new_servomotor,
    &new_lmotor,
    &new_lsmotor,
    &new_ctrlmini,
    &new_ctrlservo, /* 25 */
    &new_ctrlfplus,
    &new_smallpanel,
    &new_mpanel,
    &new_bigpanel,
    &new_wmotor, /* 30 */
    &new_battery3v,
    &new_generator,
    &new_powercable,
    &new_signalcable,
    &new_ifacecable, /* 35 */
    &new_magplug,
    &new_magsock,
    &new_switch,
    &new_interfaceclip,
    &new_gyroscope, /* 40 */
    &new_button,
    &new_xorgate,
    &new_orgate,
    &new_andgate,
    &new_nandgate, /* 45 */
    &new_invertergate,
    &new_integergate,
    &new_squaregate,
    &new_sqrtgate,
    &new_sparsifier, /* 50 */
    &new_besserwisser,
    &new_epsilon,
    &new_clamp,
    &new_toggler,
    &new_fifo, /* 55 */
    &new_mavg,
    &new_cavg,
    &new_debugger,
    &new_robot,
    &new_sticky, /* 60 */
    &new_cup,
    &new_ragdoll,
    &new_breadboard,
    &new_command,
    &new_goal, /* 65 */
    &new_gameman,
    &new_damper2,
    &new_rubberbeam,
    &new_pivot2,
    &new_corner, /* 70 */
    &new_scanner,
    &new_tiltmeter,
    &new_thruster,
    &new_rocket,
    &new_proximitysensor, /* 75 */
    &new_ysplitter,
    &new_valueshift,
    &new_togglebutton,
    &new_ifgate,
    &new_idfield, /* 80 */
    &new_interactive_cylinder,
    &new_dragfield,
    &new_emitter,
    &new_landmine,
    &new_bomb, /* 85 */
    &new_absorber,
    &new_timer,
    &new_miniemitter,
    &new_miniabsorber,
    &new_conveyor, /* 90 */
    &new_electromagnet,
    &new_plasticbeam,
    &new_woodbox,
    &new_resistor,
    &new_rubberband1, /* 95 */
    &new_rubberband2,
    &new_angulardamper,
    &new_object_finder,
    &new_pressure_sensor,
    &new_sincos, /* 100 */
    &new_sinewave,
    &new_erandom,
    &new_memory,
    &new_gravity_manager,
    &new_gravity_setter, /* 105 */
    &new_sawtooth,
    &new_jumper,
    &new_ibox,
    &new_iball,
    &new_halfunpack, /* 110 */
    &new_halfpack,
    &new_eatan2,
    &new_pointer,
    &new_spikes,
    &new_objectfield, /* 115 */
    &new_sum,
    &new_signalclip,
    &new_avg,
    &new_muladd,
    &new_gear, /* 120 */
    &new_screenshot_marker,
    &new_pixel,
    &new_receiver,
    &new_transmitter,
    &new_broadcaster, /* 125 */
    &new_fan,
    &new_min,
    &new_max,
    &new_backpack,
    &new_stabilizer, /* 130 */
    &new_pkgwarp,
    &new_pkgstatus,
    &new_camtargeter,
    &new_condenser,
    &new_fxemitter,  /* 135 */
    &new_emul,
    &new_xsmallpanel,
    &new_esub,
    &new_minitransmitter,
    &new_checkpoint, /* 140 */
    &new_spikebot,
    &new_companion,
    &new_bomber,
    &new_impact_sensor,
    &new_lobber,     /* 145 */
    &new_hpcontrol,
    &new_laser,
    &new_multiemitter,
    &new_simplemotor,
    &new_angularvelmeter, /* 150 */
    &new_mirror,
    &new_lasersensor,
    &new_velmeter,
    &new_wrapadd,
    &new_wrapsub,    /* 155 */
    &new_eventlistener,
    &new_passive_display,
    &new_ceilgate,
    &new_rcactivator,
    &new_cursor_finder, /* 160 */
    &new_autoabsorber,
    &new_wrapdist,
    &new_weight,
    &new_decay,
    &new_zoomer, /* 165 */
    &new_timectrl,
    &new_prompt,
    &new_graph,
    &new_wrapcondenser,
    &new_ifelse, /* 170 */
    &new_cmpe,
    &new_cmpl,
    &new_cmple,
    &new_sfxemitter,
    &new_speaker, /* 175 */
    &new_var_getter,
    &new_var_setter,
    &new_snap,
    &new_sequencer,
    &new_ghost, /* 180 */
    &new_ldecay,
    &new_elimit,
    &new_cursorfield,
    &new_escript,
    &new_tpixel, /* 185 */
    &new_item,
    &new_oilrig,
    &new_factory,
    &new_crane,
    &new_fluid, /* 190 */
    &new_localgravity,
    &new_autoprotector,
    &new_active_display,
    &new_pbox,
    &new_boundary, /* 195 */
    &new_robotman,
    &new_chunk,
    &new_suction_cup,
    &new_soundman,
    &new_robot_factory, /* 200!! */
    &new_minibot,
    &new_armory,
    &new_separator,
    &new_room,
    &new_oil_mixer, /* 205 */
    &new_repair_station,
    &new_guardpoint,
    &new_target_setter,
    &new_ladder,
    &new_resource, /* 210 */
    &new_vendor,
    &new_plant,
    &new_animal,
    &new_player_activator,
    &new_plastic_polygon, /* 215 */
    &new_key_listener,
    &new_statesaver,
    &new_ifselect,
    &new_camera_rotator,
    &new_level_manager, /* 220 */
    &new_treasure_chest,
    &new_decoration,
    &new_megasplitter,
    &new_ladder_step,
    &new_mini_spikebot, /* 225 */
};

static int num_creators = sizeof(c_creator)/sizeof(void*);

static const char *categories[] = {
 "Basic",
 "Mechanics",
 "Electronics",
 "Robotics",
 "Signal-i1o1",
 "Signal-i2o1",
 "Signal-misc",
 "Tools/effects",
 "Interaction",
 "Game",
};

static const char *category_hints[] = {
 "bas",
 "mech",
 "elec",
 "rob",
 "s-i1",
 "s-i2",
 "s-m",
 "t/fx",
 "i",
 "game",
};

/* Basic */
static int c0_ids[] = {
    O_PLANK,
    O_THICK_PLANK,
    O_BALL,
    O_BOX,
    O_CORNER,
    O_CYLINDER,
    O_SUBLAYER_PLANK,
    O_PLATFORM,
    O_METAL_BALL,
    O_RUBBER_BEAM,
    O_PLASTIC_BEAM,
    O_PLASTIC_BOX,
    O_PLASTIC_POLYGON,
    //O_SEPARATOR,
    //O_ROOM,
    O_PIXEL,
    O_TPIXEL,
    O_DUMMY,
    O_SPIKES,
    O_STICKY_NOTE,
    O_PLASTIC_CUP,
    O_BREADBOARD,
    O_RAIL_STRAIGHT,
    O_RAIL_UP,
    O_RAIL_DOWN,
    O_RAIL_TURN,
    O_BALL_PIPELINE,
};

/* Mechanics */
static int c1_ids[] = {
    O_TRAMPOLINE,
    O_ROPE,
    O_RUBBERBAND,
    O_WHEEL,
    O_SEESAW,
    O_OPEN_PIVOT,
    O_DAMPER,
    O_WALL_PIVOT,
    O_THRUSTER,
    O_ROCKET,
    O_CONVEYOR,
    O_FAN,
    O_BUTTON,
    O_TOGGLE_BUTTON,
    O_WEIGHT,
    O_BOMB,
    O_LAND_MINE,
    //O_GEARBOX,
    O_ANGULAR_DAMPER,
    O_GEAR,
    O_MAGNET,
    O_ELECTROMAGNET,
    // put here because of lack of space in Basic categ
    O_SEPARATOR
};

/* Electronics */
static int c2_ids[] = {
    O_BATTERY,
    O_POWER_SUPPLY,
    O_SIMPLE_MOTOR,
    O_POWER_CABLE,
    O_SIGNAL_CABLE,
    O_INTERFACE_CABLE,
    O_JUMPER,
    O_MAGNETIC_PLUG,
    O_MAGNETIC_SOCKET,
    O_INTERFACE_CLIP,
    O_SIGNALCLIP,
    O_EC_RESISTOR,
    O_RECEIVER,
    O_TRANSMITTER,
    O_BROADCASTER,
    O_MINI_TRANSMITTER,
};

/* Robotics */
static int c3_ids[] = {
    O_DC_MOTOR,
    O_SERVO_MOTOR,
    O_LINEAR_MOTOR,
    O_LINEAR_SERVO_MOTOR,
    O_CT_MINI,
    O_CT_SERVO,
    O_CT_FEEDBACK,
    O_RC_MICRO,
    O_RC_BASIC,
    O_RC_IO3,
    O_RC_MONSTRO,
    O_LASER,
    O_LASER_BOUNCER,
    O_LASER_SENSOR,
    O_PROXIMITY_SENSOR,
    O_PRESSURE_SENSOR,
    O_IMPACT_SENSOR,
    O_ANGULARVELMETER,
    O_VELMETER,
    O_TILTMETER,
    O_GYROSCOPE,
    O_OBJECT_FINDER,
    O_ID_FIELD,
    O_OBJECT_FIELD,
    O_TARGET_SETTER,
};

/* Signal-i1o1 */
static int c4_ids[] = {
    O_INVERTER,
    O_FLOOR,
    O_CEIL,
    O_SQUARE,
    O_SQRT,
    O_SPARSIFIER,
    O_SPARSIFIERPLUS,
    O_EPSILON,
    O_TOGGLER,
    O_MAVG,
    O_ZERO_RESET_MAVG,
    O_FIFO,
    O_VALUE_SHIFT,
    O_CLAMP,
    O_MULADD,
    O_ESUB,
    O_DECAY,
    O_LINEAR_DECAY,
    O_LIMIT,
    O_SNAP,
    O_BOUNDARY,
};

/* Signal-i2o1 */
static int c5_ids[] = {
    O_XORGATE,
    O_ORGATE,
    O_ANDGATE,
    O_NANDGATE,
    O_IFGATE,
    O_CMPE,
    O_CMPL,
    O_CMPLE,
    O_MIN,
    O_MAX,
    O_SUM,
    O_MUL,
    O_AVG,
    O_CONDENSER,
    O_WRAPCONDENSER,
    O_MEMORY,
    O_WRAPADD,
    O_WRAPSUB,
    O_WRAPDIST,
};

/* Signal-misc */
static int c6_ids[] = {
    O_YSPLITTER,
    O_MEGASPLITTER,
    O_IFELSE,
    O_IFSELECT,
    O_HALFUNPACK,
    O_HALFPACK,
    O_SINCOS,
    O_ATAN2,
    O_SWITCH,
    O_PASSIVE_DISPLAY,
    O_ACTIVE_DISPLAY,
    O_DEBUGGER,
    O_GRAPH,
    O_POINTER,
    O_CAMERA_ROTATOR,
    O_SINEWAVE,
    O_SAWTOOTH,
    O_RANDOM,
    O_TIMER,
    O_EVENT_LISTENER,
    O_KEY_LISTENER,
    O_HP_CONTROL,
    O_SEQUENCER,
    O_VAR_GETTER,
    O_VAR_SETTER,
};

/* Tools/effects */
static int c7_ids[] = {
    O_FX_EMITTER,
    O_SFX_EMITTER,
    O_SYNTHESIZER,
    O_TIMECTRL,
    O_GRAVITY_MANAGER,
    O_GRAVITY_SETTER,
    O_ARTIFICIAL_GRAVITY,
    O_STABILIZER,
    O_ESCRIPT,
    O_SHAPE_EXTRUDER,
    O_EMITTER,
    O_MINI_EMITTER,
    O_ABSORBER,
    O_MINI_ABSORBER,
    O_MULTI_EMITTER,
    O_AUTO_ABSORBER,
    O_AUTO_PROTECTOR,
    O_FLUID
};

/* Interaction */
static int c8_ids[] = {
    O_CURSOR_FIELD,
    O_CURSOR_FINDER,
    O_PROMPT,
    O_CAM_MARKER,
    O_CAM_TARGETER,
    O_CAM_ZOOMER,
    O_RC_ACTIVATOR,
    O_PLAYER_ACTIVATOR,
    O_INTERACTIVE_CYLINDER,
    O_INTERACTIVE_BOX,
    O_INTERACTIVE_BALL,
    O_DRAGFIELD,
    O_VENDOR,
    O_TREASURE_CHEST,
};

/* Game */
static int c9_ids[] = {
    O_ROBOT,
    O_SPIKEBOT,
    O_COMPANION,
    O_BOMBER,
    O_LOBBER,
    O_MINIBOT,
    O_ANIMAL,
    O_MINI_SPIKEBOT,
    O_LADDER,
    O_LADDER_STEP,
    O_COMMAND_PAD,
    O_GOAL,
    O_BACKPACK,
    O_CHECKPOINT,
    O_GUARDPOINT,
    O_GAMEMAN,
    O_ROBOTMAN,
    O_SOUNDMAN,
    O_LEVEL_MANAGER,
    O_PKG_WARP,
    O_PKG_STATUS,
    O_ITEM,
    O_OILRIG,
    O_FACTORY,
    O_ROBOT_FACTORY,
    O_ARMORY,
    O_OIL_MIXER,
    O_REPAIR_STATION,
    O_CRANE,
    O_SUCTION_CUP,
    O_RESOURCE,
    O_PLANT,
    O_STATE_SAVER,
    O_DECORATION,
};

static const int num_objects[of::num_categories] = {
    (sizeof(c0_ids)/sizeof(int)),
    (sizeof(c1_ids)/sizeof(int)),
    (sizeof(c2_ids)/sizeof(int)),
    (sizeof(c3_ids)/sizeof(int)),
    (sizeof(c4_ids)/sizeof(int)),
    (sizeof(c5_ids)/sizeof(int)),
    (sizeof(c6_ids)/sizeof(int)),
    (sizeof(c7_ids)/sizeof(int)),
    (sizeof(c8_ids)/sizeof(int)),
    (sizeof(c9_ids)/sizeof(int)),
};

static int *ids[] = {
    c0_ids, c1_ids, c2_ids, c3_ids, c4_ids, c5_ids, c6_ids,c7_ids,c8_ids,c9_ids
};

void
of::init(void)
{

}

int of::get_gid(int category, int child)
{
    /* TODO: bounds check */
    return ids[category][child];
}

int of::get_num_objects(int cat)
{
    return num_objects[cat];
}

const char *
of::get_category_name(int x)
{
    return categories[x];
}

const char *
of::get_category_hint(int x)
{
    return category_hints[x];
}

static entity*
_create(p_gid id)
{
    entity *e = 0;

    if (id < num_creators)
        e = ((*c_creator[id]))();

    if (e) {
        e->g_id = id;
    }

    return e;
}

item *
of::create_item(uint32_t item_id)
{
    item *e = static_cast<item*>(_create(O_ITEM));
    if (e) {
        e->id = of::get_next_id();
        e->properties[0].v.i = item_id;
    }

    return e;
}

entity*
of::create(p_gid g_id)
{
    entity *e = _create(g_id);

    if (e) {
        e->id = of::get_next_id();
    }

    return e;
}

entity*
of::create_with_id(p_gid g_id, p_id id)
{
    entity *e = _create(g_id);

    if (e) {
        e->id = id;
    }

    return e;
}

/* IMPORTANT: keep this in sync with
 * lvledit::print_gids()
 * and
 * chunk_preloader::preload_entity() */
entity*
of::read(lvlbuf *lb, uint8_t version, uint32_t id_modifier, b2Vec2 displacement, std::vector<chunk_pos> *affected_chunks)
{
    entity *e;
    p_gid g_id;
    uint8_t np, nc;
    uint32_t state_size = 0;
    uint32_t group_id;
    uint32_t id;

    /* XXX GID XXX */
    g_id = lb->r_uint8();
    id = lb->r_uint32() + id_modifier;
    group_id = (uint32_t)lb->r_uint16();
    group_id = group_id | (lb->r_uint16() << 16);

    //tms_debugf("read g_id: %u", g_id);

    if (group_id != 0) group_id += id_modifier;

    e = of::create_with_id(g_id, id);

    if (e) {
        np = lb->r_uint8();

        if (version >= LEVEL_VERSION_1_5) {
            nc = lb->r_uint8();
            state_size = lb->r_uint32();
        } else
            nc = 0;

        e->gr = (group*)(uintptr_t)group_id;

        // XXX: Should this check if id != world->level.adventure_id instead?
        // XXX: No, it should not
        if (id >= of::_id && id != 0xffffffff) of::_id = id+1;

        e->g_id = g_id;

        if (np != e->num_properties) {
            if (np > e->num_properties) {
                tms_errorf("Too many properties for object %d, will try to compensate.", e->g_id);
                //np = e->num_properties;
                //return 0;
            }
        }

        e->_pos.x = lb->r_float();
        e->_pos.y = lb->r_float();

        /* if we're grouped, the pos is local within the group, do not add displacement
         * since displacement has already been added to the group itself */
        if (e->gr == 0) {
            e->_pos.x += displacement.x;
            e->_pos.y += displacement.y;
        }

        e->_angle = lb->r_float();
        e->set_layer((int)lb->r_uint8());

        if (version >= LEVEL_VERSION_1_5) {
            e->load_flags(lb->r_uint64());

            for (int x=0; x<nc; x++) {
                int cx = lb->r_int32(); /* chunk x */
                int cy = lb->r_int32(); /* chunk y */

                if (e->flag_active(ENTITY_STATE_SLEEPING)) {
                    e->chunk_intersections[x].x = cx;
                    e->chunk_intersections[x].y = cy;
                    e->chunk_intersections[x].num_fixtures = 1;
                    e->num_chunk_intersections++;
                }

                /* if we have a pointer to an "affected chunks" vector, insert info there */
                if (affected_chunks) {
                    affected_chunks->push_back(chunk_pos(cx, cy));
                }
            }

            e->state_size = state_size;
            e->state_ptr = lb->rp;
            lb->rp += state_size;
        } else {
            e->set_flag(ENTITY_AXIS_ROT, (bool)lb->r_uint8());

            if (version >= 10) {
                e->set_moveable((bool)lb->r_uint8());
            } else {
                e->set_moveable(true);
            }
        }

        for (int x=0; x<np; x++) {
            uint8_t type = lb->r_uint8();

            if (x >= e->num_properties) {
                tms_infof("Skipping property, type %d", type);
                switch (type) {
                    case P_INT8: lb->r_uint8(); break;
                    case P_INT: lb->r_uint32(); break;
                    case P_ID: lb->r_uint32(); break;
                    case P_FLT: lb->r_float(); break;
                    case P_STR:
                        {
                            uint32_t len;
                            if (version >= LEVEL_VERSION_1_5) {
                                len = lb->r_uint32();
                            } else {
                                len = lb->r_uint16();
                            }

                            lb->rp += len;
                        }
                        break;
                }
                continue;
            }

            property *p = &e->properties[x];

            if (type != p->type) {
                if (p->type == P_FLT && type == P_INT8) {
                    uint8_t v = lb->r_uint8();
                    p->v.f = (float)v;

                    tms_infof("Read uint8 %u from file, converted it to float %f", v, p->v.f);
                } else if (p->type == P_FLT && type == P_INT) {
                    uint32_t v = lb->r_uint32();
                    p->v.f = (float)v;

                    tms_infof("Read uint32 %u from file, converted it to float %f", v, p->v.f);
                } else if (p->type == P_INT && type == P_INT8) {
                    uint8_t v = lb->r_uint8();
                    p->v.i = (uint32_t)v;

                    tms_infof("Read uint8 %d from file, converted it to uint32 %d", v, p->v.i);
                } else if (p->type == P_INT8 && type == P_INT) {
                    uint32_t v = lb->r_uint32();
                    p->v.i8 = (uint8_t)v;
                    tms_infof("Read uint32 %d from file, converted it to uint8 %d", v, p->v.i8);
                } else if (type == P_INT8 || p->type == P_INT8 || p->type == P_STR || type == P_STR) {
                    switch (type) {
                        case P_INT8: lb->r_uint8(); break;
                        case P_INT: lb->r_uint32(); break;
                        case P_ID: lb->r_uint32(); break;
                        case P_FLT: lb->r_float(); break;
                        case P_STR:
                                    {
                                        uint32_t len = 0;
                                        if (version >= LEVEL_VERSION_1_5) {
                                            len = lb->r_uint32();
                                        } else {
                                            len = lb->r_uint16();
                                        }
                                        char *buf = (char*)malloc(len);
                                        lb->r_buf(buf, len);
                                        free(buf);
                                    }
                                    break;
                        default: tms_fatalf("invalid object property %d", type);
                    }
                    /* TODO: Should it gracefully quit? This should be an error
                     *       that will sort itself out when the level is re-saved */
                    tms_errorf("incorrect property type when loading properties");
                    memset(&p->v, 0, sizeof(p->v));
                } else if ((p->type == P_INT && type == P_ID)
                        || (p->type == P_ID && type == P_INT)) {
                    p->v.i = lb->r_uint32();
                }
            } else {
                switch (type) {
                    case P_INT8: p->v.i8 = lb->r_uint8(); break;
                    case P_INT: p->v.i = lb->r_uint32(); break;
                    case P_ID: p->v.i = lb->r_uint32() + id_modifier; break;
                    //case P_ID: p->v.i = lb->r_uint32(); break;
                    case P_FLT: p->v.f = lb->r_float(); break;
                    case P_STR:
                        if (version >= LEVEL_VERSION_1_5) {
                            p->v.s.len = lb->r_uint32();
                        } else {
                            p->v.s.len = lb->r_uint16();
                        }
                        p->v.s.buf = (char*)malloc(p->v.s.len+1);
                        lb->r_buf(p->v.s.buf, p->v.s.len);
                        p->v.s.buf[p->v.s.len] = '\0';
                        break;

                    default:
                        tms_fatalf("invalid object property %d", type);
                }
            }
        }
    } else {
        tms_errorf("invalid object: %d", g_id);
    }

    return e;
}

group *
of::read_group(lvlbuf *lb, uint8_t version, uint32_t id_modifier, b2Vec2 displacement)
{
    /* XXX keep in sync with chunk_preload::read_group() */

    group *g = new group();
    g->id = lb->r_uint32() + id_modifier;
    g->_pos.x = lb->r_float() + displacement.x;
    g->_pos.y = lb->r_float() + displacement.y;
    g->_angle = lb->r_float();

    if (version >= LEVEL_VERSION_1_5) {
        g->state_size = lb->r_uint32();
        g->state_ptr = lb->rp;
        lb->rp += g->state_size;
    }

    if (g->id >= of::_id) of::_id = g->id+1;

    return g;
}

void
of::write_group(lvlbuf *lb, uint8_t version, group *e, uint32_t id_modifier, b2Vec2 displacement, bool write_states/*=false*/)
{
    lb->ensure(
             4 /* id */
            +4 /* pos x */
            +4 /* pos y */
            +4 /* angle */
            +4 /* state_size */
            );

    b2Vec2 p = e->get_position();

    e->write_ptr = lb->size;

    lb->w_uint32(e->id + id_modifier);
    lb->w_float(p.x + displacement.x);
    lb->w_float(p.y + displacement.y);
    lb->w_float(e->get_angle());

    uint32_t state_size_ptr = 0;
    if (version >= LEVEL_VERSION_1_5) {
        state_size_ptr = lb->size;
        lb->w_uint32(0);

        if (write_states) {
            size_t before_state = lb->size;

            e->write_state(0, lb);

            size_t after_state = lb->size;
            size_t state_size = after_state - before_state;

            lb->size = state_size_ptr;
            lb->w_uint32(state_size);

            lb->size = after_state;
        }
    }

    e->write_size = lb->size - e->write_ptr;
}

void
of::write(lvlbuf *lb, uint8_t version, entity *e, uint32_t id_modifier, b2Vec2 displacement, bool write_states/*=false*/)
{
    e->write_ptr = lb->size;

    /* XXX GID XXX */
    /* we always ensure the "worst case scenario" */
    lb->ensure(1 /* uint8_t, g_id */
              +4 /* uint32_t, id */
              +2 /* uint16_t, group id low */
              +2 /* uint16_t, group id high */
              +1 /* uint8_t, num properties */
              +1 /* uint8_t, num_chunk_intersections */
              +4 /* uint32_t, state_size */
              +4 /* float, pos.x */
              +4 /* float, pos.y */
              +4 /* float, angle */
              +1 /* uint8_t, layer */
              +8 /* uint64_t, flags */
              +1 /* some remaining stuff we might as well leave ;-) */
              +1
              +1
              +8
              +4
            );

    /* XXX GID XXX */
    lb->w_uint8(e->g_id);
    lb->w_uint32(e->id + id_modifier);

    uint32_t group_id = (e->gr ? e->gr->id + id_modifier : 0);
    uint16_t group_id_low = (uint16_t)(group_id & 0xffff);
    uint16_t group_id_high = (uint16_t)((group_id >> 16) & 0xffff);
    //tms_infof("write entity %u, group %u", e->id+id_modifier, group_id);

    lb->w_uint16(group_id_low);
    lb->w_uint16(group_id_high); /* unused, was breadboard id */

    lb->w_uint8(e->num_properties);

    uint32_t state_size_ptr = 0;

    if (version >= LEVEL_VERSION_1_5) {
        lb->w_uint8(e->num_chunk_intersections);

        state_size_ptr = lb->size;
        lb->w_uint32(0);
    }

    /* only add displacement if we're not in a group,
     * since we have local coordinates if we're in a group */
    lb->w_float(e->_pos.x + (e->gr == 0 ? displacement.x : 0.f));
    lb->w_float(e->_pos.y + (e->gr == 0 ? displacement.y : 0.f));
    lb->w_float(e->_angle);
    lb->w_uint8((uint8_t)e->get_layer());

    if (version >= LEVEL_VERSION_1_5) {
        lb->w_uint64(e->save_flags());

        /* write list of chunks */
        lb->ensure(e->num_chunk_intersections * sizeof(uint32_t) * 2);

        for (int x=0; x<e->num_chunk_intersections; x++) {
#ifdef DEBUG_PRELOADER_SANITY
            tms_assertf(std::abs(e->chunk_intersections[x].x) < 1000 && std::abs(e->chunk_intersections[x].y) < 1000,
                    "suspicious chunk intersection during write");
#endif
            lb->w_int32(e->chunk_intersections[x].x);
            lb->w_int32(e->chunk_intersections[x].y);
        }

        if (write_states) {
            size_t before_state = lb->size;

            e->write_state(0, lb);

            size_t after_state = lb->size;
            size_t state_size = after_state - before_state;

            /* go back and fill in the state size */
            lb->size = state_size_ptr;
            lb->w_uint32(state_size);

            /* return to front */
            lb->size = after_state;
        }
    } else {
        lb->w_uint8((uint8_t)e->flag_active(ENTITY_AXIS_ROT));

        if (version >= 10) {
            lb->w_uint8((uint8_t)e->is_moveable());
        }
    }

    for (int x=0; x<e->num_properties; x++) {
        property *p = &e->properties[x];
        lb->w_s_uint8(p->type);

        switch (p->type) {
            case P_INT8: lb->w_s_uint8(p->v.i8); break;
            case P_INT: lb->w_s_uint32(p->v.i); break;
            case P_ID:  lb->w_s_uint32(p->v.i + id_modifier); break;
            //case P_ID:  lb->w_s_uint32(p->v.i); break;
            case P_FLT: lb->w_s_float(p->v.f); break;
            case P_STR:
                if (version >= LEVEL_VERSION_1_5) {
                    lb->w_s_uint32(p->v.s.len);
                } else {
                    lb->w_s_uint16(p->v.s.len);
                }

                lb->w_s_buf(p->v.s.buf, p->v.s.len);
                break;

            default:
                tms_errorf("invalid property type");
                break;
        }
    }

    e->write_size = lb->size - e->write_ptr;
}

uint32_t
of::get_next_id(void)
{
    return of::_id++;
}

const char *
of::get_object_name_by_gid(uint32_t gid)
{
    // ~~oh dear I think this is hacky~~
    // The old implementation would read from object help text data, but since that
    // is gone now, this was the best I could come up with. This method is basically
    // only used for factory objects and debugging stuff.
    entity *obj = _create(gid);
    if (obj) {
        const char* obj_name = obj->get_name();
        delete obj;
        return obj_name;
    } else {
        return "Nonexistant object";
    }
}
