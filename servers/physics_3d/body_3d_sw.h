/*************************************************************************/
/*  body_3d_sw.h                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef BODY_SW_H
#define BODY_SW_H

#include "area_3d_sw.h"
#include "collision_object_3d_sw.h"
#include "core/templates/vset.h"

class Constraint3DSW;

class Body3DSW : public CollisionObject3DSW {
	PhysicsServer3D::BodyMode mode;

	Vector3 linear_velocity;
	Vector3 angular_velocity;

	Vector3 biased_linear_velocity;
	Vector3 biased_angular_velocity;
	real_t mass;
	real_t bounce;
	real_t friction;

	real_t linear_damp;
	real_t angular_damp;
	real_t gravity_scale;

	uint16_t locked_axis = 0;

	real_t _inv_mass;
	Vector3 _inv_inertia; // Relative to the principal axes of inertia

	// Relative to the local frame of reference
	Basis principal_inertia_axes_local;
	Vector3 center_of_mass_local;

	// In world orientation with local origin
	Basis _inv_inertia_tensor;
	Basis principal_inertia_axes;
	Vector3 center_of_mass;

	Vector3 gravity;

	real_t still_time;

	Vector3 applied_force;
	Vector3 applied_torque;

	real_t area_angular_damp;
	real_t area_linear_damp;

	SelfList<Body3DSW> active_list;
	SelfList<Body3DSW> inertia_update_list;
	SelfList<Body3DSW> direct_state_query_list;

	VSet<RID> exceptions;
	bool omit_force_integration;
	bool active;

	bool first_integration;

	bool continuous_cd;
	bool can_sleep;
	bool first_time_kinematic;
	void _update_inertia();
	virtual void _shapes_changed();
	Transform3D new_transform;

	Map<Constraint3DSW *, int> constraint_map;

	Vector<AreaCMP> areas;

	struct Contact {
		Vector3 local_pos;
		Vector3 local_normal;
		real_t depth;
		int local_shape;
		Vector3 collider_pos;
		int collider_shape;
		ObjectID collider_instance_id;
		RID collider;
		Vector3 collider_velocity_at_pos;
	};

	Vector<Contact> contacts; //no contacts by default
	int contact_count;

	struct ForceIntegrationCallback {
		Callable callable;
		Variant udata;
	};

	ForceIntegrationCallback *fi_callback;

	uint64_t island_step;

	_FORCE_INLINE_ void _compute_area_gravity_and_dampenings(const Area3DSW *p_area);

	_FORCE_INLINE_ void _update_transform_dependant();

	friend class PhysicsDirectBodyState3DSW; // i give up, too many functions to expose

public:
	void set_force_integration_callback(const Callable &p_callable, const Variant &p_udata = Variant());

	_FORCE_INLINE_ void add_area(Area3DSW *p_area) {
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount += 1;
		} else {
			areas.ordered_insert(AreaCMP(p_area));
		}
	}

	_FORCE_INLINE_ void remove_area(Area3DSW *p_area) {
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount -= 1;
			if (areas[index].refCount < 1) {
				areas.remove(index);
			}
		}
	}

	_FORCE_INLINE_ void set_max_contacts_reported(int p_size) {
		contacts.resize(p_size);
		contact_count = 0;
		if (mode == PhysicsServer3D::BODY_MODE_KINEMATIC && p_size) {
			set_active(true);
		}
	}
	_FORCE_INLINE_ int get_max_contacts_reported() const { return contacts.size(); }

	_FORCE_INLINE_ bool can_report_contacts() const { return !contacts.is_empty(); }
	_FORCE_INLINE_ void add_contact(const Vector3 &p_local_pos, const Vector3 &p_local_normal, real_t p_depth, int p_local_shape, const Vector3 &p_collider_pos, int p_collider_shape, ObjectID p_collider_instance_id, const RID &p_collider, const Vector3 &p_collider_velocity_at_pos);

	_FORCE_INLINE_ void add_exception(const RID &p_exception) { exceptions.insert(p_exception); }
	_FORCE_INLINE_ void remove_exception(const RID &p_exception) { exceptions.erase(p_exception); }
	_FORCE_INLINE_ bool has_exception(const RID &p_exception) const { return exceptions.has(p_exception); }
	_FORCE_INLINE_ const VSet<RID> &get_exceptions() const { return exceptions; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }
	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ void add_constraint(Constraint3DSW *p_constraint, int p_pos) { constraint_map[p_constraint] = p_pos; }
	_FORCE_INLINE_ void remove_constraint(Constraint3DSW *p_constraint) { constraint_map.erase(p_constraint); }
	const Map<Constraint3DSW *, int> &get_constraint_map() const { return constraint_map; }
	_FORCE_INLINE_ void clear_constraint_map() { constraint_map.clear(); }

	_FORCE_INLINE_ void set_omit_force_integration(bool p_omit_force_integration) { omit_force_integration = p_omit_force_integration; }
	_FORCE_INLINE_ bool get_omit_force_integration() const { return omit_force_integration; }

	_FORCE_INLINE_ Basis get_principal_inertia_axes() const { return principal_inertia_axes; }
	_FORCE_INLINE_ Vector3 get_center_of_mass() const { return center_of_mass; }
	_FORCE_INLINE_ Vector3 xform_local_to_principal(const Vector3 &p_pos) const { return principal_inertia_axes_local.xform(p_pos - center_of_mass_local); }

	_FORCE_INLINE_ void set_linear_velocity(const Vector3 &p_velocity) { linear_velocity = p_velocity; }
	_FORCE_INLINE_ Vector3 get_linear_velocity() const { return linear_velocity; }

	_FORCE_INLINE_ void set_angular_velocity(const Vector3 &p_velocity) { angular_velocity = p_velocity; }
	_FORCE_INLINE_ Vector3 get_angular_velocity() const { return angular_velocity; }

	_FORCE_INLINE_ const Vector3 &get_biased_linear_velocity() const { return biased_linear_velocity; }
	_FORCE_INLINE_ const Vector3 &get_biased_angular_velocity() const { return biased_angular_velocity; }

	_FORCE_INLINE_ void apply_central_impulse(const Vector3 &p_impulse) {
		linear_velocity += p_impulse * _inv_mass;
	}

	_FORCE_INLINE_ void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3()) {
		linear_velocity += p_impulse * _inv_mass;
		angular_velocity += _inv_inertia_tensor.xform((p_position - center_of_mass).cross(p_impulse));
	}

	_FORCE_INLINE_ void apply_torque_impulse(const Vector3 &p_impulse) {
		angular_velocity += _inv_inertia_tensor.xform(p_impulse);
	}

	_FORCE_INLINE_ void apply_bias_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3(), real_t p_max_delta_av = -1.0) {
		biased_linear_velocity += p_impulse * _inv_mass;
		if (p_max_delta_av != 0.0) {
			Vector3 delta_av = _inv_inertia_tensor.xform((p_position - center_of_mass).cross(p_impulse));
			if (p_max_delta_av > 0 && delta_av.length() > p_max_delta_av) {
				delta_av = delta_av.normalized() * p_max_delta_av;
			}
			biased_angular_velocity += delta_av;
		}
	}

	_FORCE_INLINE_ void apply_bias_torque_impulse(const Vector3 &p_impulse) {
		biased_angular_velocity += _inv_inertia_tensor.xform(p_impulse);
	}

	_FORCE_INLINE_ void add_central_force(const Vector3 &p_force) {
		applied_force += p_force;
	}

	_FORCE_INLINE_ void add_force(const Vector3 &p_force, const Vector3 &p_position = Vector3()) {
		applied_force += p_force;
		applied_torque += (p_position - center_of_mass).cross(p_force);
	}

	_FORCE_INLINE_ void add_torque(const Vector3 &p_torque) {
		applied_torque += p_torque;
	}

	void set_active(bool p_active);
	_FORCE_INLINE_ bool is_active() const { return active; }

	_FORCE_INLINE_ void wakeup() {
		if ((!get_space()) || mode == PhysicsServer3D::BODY_MODE_STATIC || mode == PhysicsServer3D::BODY_MODE_KINEMATIC) {
			return;
		}
		set_active(true);
	}

	void set_param(PhysicsServer3D::BodyParameter p_param, real_t);
	real_t get_param(PhysicsServer3D::BodyParameter p_param) const;

	void set_mode(PhysicsServer3D::BodyMode p_mode);
	PhysicsServer3D::BodyMode get_mode() const;

	void set_state(PhysicsServer3D::BodyState p_state, const Variant &p_variant);
	Variant get_state(PhysicsServer3D::BodyState p_state) const;

	void set_applied_force(const Vector3 &p_force) { applied_force = p_force; }
	Vector3 get_applied_force() const { return applied_force; }

	void set_applied_torque(const Vector3 &p_torque) { applied_torque = p_torque; }
	Vector3 get_applied_torque() const { return applied_torque; }

	_FORCE_INLINE_ void set_continuous_collision_detection(bool p_enable) { continuous_cd = p_enable; }
	_FORCE_INLINE_ bool is_continuous_collision_detection_enabled() const { return continuous_cd; }

	void set_space(Space3DSW *p_space);

	void update_inertias();

	_FORCE_INLINE_ real_t get_inv_mass() const { return _inv_mass; }
	_FORCE_INLINE_ const Vector3 &get_inv_inertia() const { return _inv_inertia; }
	_FORCE_INLINE_ const Basis &get_inv_inertia_tensor() const { return _inv_inertia_tensor; }
	_FORCE_INLINE_ real_t get_friction() const { return friction; }
	_FORCE_INLINE_ const Vector3 &get_gravity() const { return gravity; }
	_FORCE_INLINE_ real_t get_bounce() const { return bounce; }

	void set_axis_lock(PhysicsServer3D::BodyAxis p_axis, bool lock);
	bool is_axis_locked(PhysicsServer3D::BodyAxis p_axis) const;

	void integrate_forces(real_t p_step);
	void integrate_velocities(real_t p_step);

	_FORCE_INLINE_ Vector3 get_velocity_in_local_point(const Vector3 &rel_pos) const {
		return linear_velocity + angular_velocity.cross(rel_pos - center_of_mass);
	}

	_FORCE_INLINE_ real_t compute_impulse_denominator(const Vector3 &p_pos, const Vector3 &p_normal) const {
		Vector3 r0 = p_pos - get_transform().origin - center_of_mass;

		Vector3 c0 = (r0).cross(p_normal);

		Vector3 vec = (_inv_inertia_tensor.xform_inv(c0)).cross(r0);

		return _inv_mass + p_normal.dot(vec);
	}

	_FORCE_INLINE_ real_t compute_angular_impulse_denominator(const Vector3 &p_axis) const {
		return p_axis.dot(_inv_inertia_tensor.xform_inv(p_axis));
	}

	//void simulate_motion(const Transform3D& p_xform,real_t p_step);
	void call_queries();
	void wakeup_neighbours();

	bool sleep_test(real_t p_step);

	Body3DSW();
	~Body3DSW();
};

//add contact inline

void Body3DSW::add_contact(const Vector3 &p_local_pos, const Vector3 &p_local_normal, real_t p_depth, int p_local_shape, const Vector3 &p_collider_pos, int p_collider_shape, ObjectID p_collider_instance_id, const RID &p_collider, const Vector3 &p_collider_velocity_at_pos) {
	int c_max = contacts.size();

	if (c_max == 0) {
		return;
	}

	Contact *c = contacts.ptrw();

	int idx = -1;

	if (contact_count < c_max) {
		idx = contact_count++;
	} else {
		real_t least_depth = 1e20;
		int least_deep = -1;
		for (int i = 0; i < c_max; i++) {
			if (i == 0 || c[i].depth < least_depth) {
				least_deep = i;
				least_depth = c[i].depth;
			}
		}

		if (least_deep >= 0 && least_depth < p_depth) {
			idx = least_deep;
		}
		if (idx == -1) {
			return; //none least deepe than this
		}
	}

	c[idx].local_pos = p_local_pos;
	c[idx].local_normal = p_local_normal;
	c[idx].depth = p_depth;
	c[idx].local_shape = p_local_shape;
	c[idx].collider_pos = p_collider_pos;
	c[idx].collider_shape = p_collider_shape;
	c[idx].collider_instance_id = p_collider_instance_id;
	c[idx].collider = p_collider;
	c[idx].collider_velocity_at_pos = p_collider_velocity_at_pos;
}

class PhysicsDirectBodyState3DSW : public PhysicsDirectBodyState3D {
	GDCLASS(PhysicsDirectBodyState3DSW, PhysicsDirectBodyState3D);

public:
	static PhysicsDirectBodyState3DSW *singleton;
	Body3DSW *body;
	real_t step;

	virtual Vector3 get_total_gravity() const override { return body->gravity; } // get gravity vector working on this body space/area
	virtual real_t get_total_angular_damp() const override { return body->area_angular_damp; } // get density of this body space/area
	virtual real_t get_total_linear_damp() const override { return body->area_linear_damp; } // get density of this body space/area

	virtual Vector3 get_center_of_mass() const override { return body->get_center_of_mass(); }
	virtual Basis get_principal_inertia_axes() const override { return body->get_principal_inertia_axes(); }

	virtual real_t get_inverse_mass() const override { return body->get_inv_mass(); } // get the mass
	virtual Vector3 get_inverse_inertia() const override { return body->get_inv_inertia(); } // get density of this body space
	virtual Basis get_inverse_inertia_tensor() const override { return body->get_inv_inertia_tensor(); } // get density of this body space

	virtual void set_linear_velocity(const Vector3 &p_velocity) override { body->set_linear_velocity(p_velocity); }
	virtual Vector3 get_linear_velocity() const override { return body->get_linear_velocity(); }

	virtual void set_angular_velocity(const Vector3 &p_velocity) override { body->set_angular_velocity(p_velocity); }
	virtual Vector3 get_angular_velocity() const override { return body->get_angular_velocity(); }

	virtual void set_transform(const Transform3D &p_transform) override { body->set_state(PhysicsServer3D::BODY_STATE_TRANSFORM, p_transform); }
	virtual Transform3D get_transform() const override { return body->get_transform(); }

	virtual Vector3 get_velocity_at_local_position(const Vector3 &p_position) const override { return body->get_velocity_in_local_point(p_position); }

	virtual void add_central_force(const Vector3 &p_force) override { body->add_central_force(p_force); }
	virtual void add_force(const Vector3 &p_force, const Vector3 &p_position = Vector3()) override {
		body->add_force(p_force, p_position);
	}
	virtual void add_torque(const Vector3 &p_torque) override { body->add_torque(p_torque); }
	virtual void apply_central_impulse(const Vector3 &p_impulse) override { body->apply_central_impulse(p_impulse); }
	virtual void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3()) override {
		body->apply_impulse(p_impulse, p_position);
	}
	virtual void apply_torque_impulse(const Vector3 &p_impulse) override { body->apply_torque_impulse(p_impulse); }

	virtual void set_sleep_state(bool p_sleep) override { body->set_active(!p_sleep); }
	virtual bool is_sleeping() const override { return !body->is_active(); }

	virtual int get_contact_count() const override { return body->contact_count; }

	virtual Vector3 get_contact_local_position(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector3());
		return body->contacts[p_contact_idx].local_pos;
	}
	virtual Vector3 get_contact_local_normal(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector3());
		return body->contacts[p_contact_idx].local_normal;
	}
	virtual real_t get_contact_impulse(int p_contact_idx) const override {
		return 0.0f; // Only implemented for bullet
	}
	virtual int get_contact_local_shape(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, -1);
		return body->contacts[p_contact_idx].local_shape;
	}

	virtual RID get_contact_collider(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, RID());
		return body->contacts[p_contact_idx].collider;
	}
	virtual Vector3 get_contact_collider_position(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector3());
		return body->contacts[p_contact_idx].collider_pos;
	}
	virtual ObjectID get_contact_collider_id(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, ObjectID());
		return body->contacts[p_contact_idx].collider_instance_id;
	}
	virtual int get_contact_collider_shape(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, 0);
		return body->contacts[p_contact_idx].collider_shape;
	}
	virtual Vector3 get_contact_collider_velocity_at_position(int p_contact_idx) const override {
		ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector3());
		return body->contacts[p_contact_idx].collider_velocity_at_pos;
	}

	virtual PhysicsDirectSpaceState3D *get_space_state() override;

	virtual real_t get_step() const override { return step; }
	PhysicsDirectBodyState3DSW() {
		singleton = this;
		body = nullptr;
	}
};

#endif // BODY__SW_H
