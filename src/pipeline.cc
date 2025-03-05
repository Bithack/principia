#include "pipeline.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "ball.hh"
#include "game.hh"

#define PIPELINE_SPEED .1f

pipeline::pipeline()
{
    this->set_flag(ENTITY_ALLOW_AXIS_ROT,       true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_IS_MAGNETIC,          true);

    this->width = .5f;
    this->type = ENTITY_PIPELINE;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->used = false;
    this->b = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_PIPELINE));
    this->set_material(&m_iron);

    this->house_open = 0.f;
    this->house = tms_entity_alloc();
    this->piston = tms_entity_alloc();
    tms_entity_set_mesh(this->house, mesh_factory::get_mesh(MODEL_PIPELINE_HOUSE));
    tms_entity_set_mesh(this->piston, mesh_factory::get_mesh(MODEL_PIPELINE_PISTON));
    tms_entity_set_material(this->house, &m_iron);
    tms_entity_set_material(this->piston, &m_iron);

    tms_entity_add_child(this, this->house);
    tms_entity_add_child(this, this->piston);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->query_vec = b2Vec2(0, -.5f);
}

void
pipeline::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));
}

void
pipeline::update()
{
    tmat4_load_identity(this->M);
    entity_fast_update(static_cast<struct tms_entity*>(this));

    if (this->flag_active(ENTITY_AXIS_ROT))
        tmat4_rotate(this->M, 180.f, 0.f, 1.f, 0.f);

    tmat4_copy(this->house->M, this->M);
    tmat4_copy(this->piston->M, this->M);

    tmat3_copy_mat4_sub3x3(this->house->N, this->house->M);
    tmat3_copy_mat4_sub3x3(this->piston->N, this->piston->M);

    tmat4_translate(this->house->M, 0.f, 0.f, -.5f * (1.f-house_open*0.f));
    tmat4_scale(this->house->M, 1.f, 1.f, .1f + house_open*0.f);

    tmat4_translate(this->piston->M, 0.f, 0.f, -.5f);

    if (this->b && this->s > 4) {
        if (this->flag_active(ENTITY_AXIS_ROT))
            tmat4_translate(this->piston->M, 0.f, 0.f, -(this->b->target_z - this->b->z) * .5f);
        else
            tmat4_translate(this->piston->M, 0.f, 0.f, (this->b->target_z - this->b->z) * .5f);
    }
}

void
pipeline::add_to_world()
{
    this->create_rect(this->get_dynamic_type(), .375f, .375f, this->material);
    this->used = false;
    this->b = 0;
    this->s = 0;
}

void
pipeline::step(void)
{
    if (this->b) {
        //tms_infof("step %d", this->s);

        switch (this->s) {
            case -1:
                /* we're locked, attempting to move the ball to a non-existing layer */
                /* do nothing... */
                break;

            case 0:
                {
                    float dist = (this->get_position() - this->b->get_position()).Length();
                    if (dist < .2f) {
                        b2WeldJointDef wjd;
                        wjd.bodyA = this->get_body(0);
                        wjd.bodyB = this->b->get_body(0);
                        wjd.localAnchorA=b2Vec2(0,0);
                        wjd.localAnchorB=b2Vec2(0,0);
                        //wjd.length = 0.f;
                        wjd.frequencyHz = 6.f;
                        wjd.dampingRatio = 2.f;
                        this->joint = W->b2->CreateJoint(&wjd);
                        s = 1;
                    } else if (dist > .75f) {
                        this->used = 0;
                        this->b = 0;
                        this->s = 0;
                        //tms_infof("ball escaped");
                    }
                }
                break;

            case 1:
                //tms_infof("step 1");
                if ((this->get_position() - this->b->get_position()).Length() < .05f) {
                    s = 2;
                }
                break;

            case 2:
                //tms_infof("step 2");
                {
                    int layer = this->b->get_layer();
                    int olayer = layer;

                    if (this->flag_active(ENTITY_AXIS_ROT))
                        layer -= 1;
                    else
                        layer += 1;
                    if (layer > 2) layer = 2;
                    if (layer < 0) layer = 0;

                    if (layer == olayer) {
                        /* unable to move */
                        s = -1;
                        break;
                    }

                    b->z = olayer;
                    b->target_z = layer;

                    //tms_infof("new layer %d", layer);

                    this->b->get_body(0)->GetFixtureList()->SetFilterData(
                            world::get_filter_for_layer(layer, 6)
                        );

                    this->b->get_body(0)->GetFixtureList()->SetSensor(true);
                    //this->b->get_body(0)->SetAngularVelocity(0.f);

                    G->remove_entity(b);
                    ((struct tms_entity *)b)->update = ball_update_customz;
                    b->curr_update_method = ENTITY_UPDATE_CUSTOM;
                    G->add_entity(b);

                    s++;
                }
                break;

            case 3:
                s++;
                break;

            case 4:
                /* wait for everything to move out of the way */
                {
                    b2ContactEdge *c;
                    b2Body *b = this->b->get_body(0);

                    int num = 0;
                    bool self = false;

                    for (c = b->GetContactList(); c; c=c->next) {
                        if (this->get_body(0) == c->other) 
                            self = true;
                        else {
                            b2Fixture *fa = c->contact->GetFixtureA();
                            b2Fixture *fb = c->contact->GetFixtureB();
                            entity *ea = (entity*)fa->GetUserData();
                            entity *eb = (entity*)fb->GetUserData();

                            if (c->contact->IsTouching()) {
                                if (this->b == ea && !fb->IsSensor())
                                    num++;
                                else if (this->b == eb && !fa->IsSensor())
                                    num++;
                            }
                        }
                    }

                    //tms_infof("step 3 num contacts %d, self %d", num, self);

                    if (!num || (num == 1 && self)) {
                        this->b->get_body(0)->GetFixtureList()->SetSensor(false);
                        s++;
                        house_open = 1.f;
                    }
                }
                break;

            case 5:
                b->z = b->z * (1.f - PIPELINE_SPEED) + b->target_z * PIPELINE_SPEED;

                if (fabsf(b->z - b->target_z) < .5f) {
                    s++;
                    /* halfway through, destroy joint */
                    W->b2->DestroyJoint(this->joint);
                    this->b->get_body(0)->SetAngularVelocity(0.f);
                }
                break;

            case 6:
                b->z = b->z * (1.f - PIPELINE_SPEED) + b->target_z * PIPELINE_SPEED;

                if (fabsf(b->z - b->target_z) < .05f) {
                    b->z = b->target_z;
                    s ++;
                }

                break;

            case 7:
                /*
                tms_infof("step 6");
                s++;
                break;
                */

            default:
                this->used = false;

                if (this->flag_active(ENTITY_AXIS_ROT))
                    this->b->layermove(-1);
                else
                    this->b->layermove(+1);

                this->b = 0;
                break;
        }

        if (this->s > 4) {
            this->house_open -= .03f;
            if (this->house_open < 0.f) this->house_open = 0.f;
        } else {
            if (this->s > 1) {
                this->house_open += .05f;
                if (this->house_open > 1.f) this->house_open = 1.f;
            }
        }
    } else {
        this->house_open -= .02f;
        if (this->house_open < 0.f) this->house_open = 0.f;
    }
}

void
pipeline::take(ball *b)
{
    //tms_infof("taking ball %p", b);
    this->used = true;
    this->s = 0; 
    this->b = b;
    this->b->target_z = this->b->z = (float)this->b->get_layer();
}
