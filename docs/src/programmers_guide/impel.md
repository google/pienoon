Impel Animation
===============

Impel Animation    {#pie_noon_guide_impel}
===============

Impel is a small, simple, procedural animation system. It's designed to be,
  * **scalable** -- data is stored centrally and processed in bulk so that we
    can easily take advantage of SIMD and multi-threading opportunities.
  * **expandable** -- new animation algorithms can be registered and executed
    without touching central code
  * **adaptable** -- the external interface is templatized to return your
    math vector types.

# Motivation    {#pie_noon_guide_impel_motivation}

Static objects may look good in screenshots, but motion is what brings a scene
to life. If done well, even simple motion along one axis can add character and
charm to your game.

The difficulty is in doing the motion well. There is a lot of subtly to motion.
There's a lot that can look ok, but not great, for reasons that are not obvious.

For these two reasons, a simple animation library is valuable--just as valuable,
perhaps, as a simple rendering or audio library.

# Example Usage   {#pie_noon_guide_impel_example_usage}

The following example illustrates how to animate the face-angle of a character.
The comments describe, in brief, what is going on internally.

~~~{.cpp}
// The engine is the central place where animation data is stored and processed.
impel::ImpelEngine engine;

// In this example, we animate a one-dimensional floating point value.
// It's also possible to animate a mathfu::vec2 with impel::Impeller2f, and
// similarly for higher dimensional vectors. We can even animate a mathfu::mat4
// (a 4x4 matrix) with impel::ImpellerMatrix4f.
//
// If you have your own math library, you can also animate those instead of
// mathfu types. See [Using Your Own Math Types][].
impel::Impeller1f facing_angle;

// Initialize facing_angle impeller to animate as a 'Smooth' impeller.
// Alternatively, we could initialize as an 'Overshoot' impeller. All impeller
// types have the same interface. Internally, they are animated with different
// algorithms, and they will move differently towards their targets. However,
// to switch between impeller types it is a simple matter of initializing with
// a different kind of ImpelInit struct.
impel::SmoothImpelInit init;
init.modular = true;   // Angles wrap around. That is, -pi is equivalent to pi.
init.min = -fpl::kPi;  // Valid range for angles is -pi..pi, inclusive of
init.max = fpl::kPi;   // +pi and exclusive of -pi.
facing_angle.Initialize(init, engine);

// Set initial state of the impeller, and the target parameters.
// 'Smooth' impellers animate to a target-value in a target-time. Not all types
// of impellers use all target data.
facing_angle.SetValue(start_angle);
facing_angle.SetVelocity(start_angular_velocity);
facing_angle.SetTargetValue(target_angle);
facing_angle.SetTargetTime(target_time);

for (impel::ImpelTime t = 0; t <= target_time; t += delta_time) {
  // The entire is updated in one call. That is, all impellers created with
  // 'engine' are animated here. This allows us to more easily take advantage
  // of SIMD or multi-threading opportunities.
  engine.AdvanceFrame(delta_time);

  // The current value of the variable being animated is always available.
  // Additionally, we can also access facing_angle.Velocity() for the angular
  // velocity.
  fpl::Angle current_facing_angle(facing_angle.Value());
  printf("facing angle at time %d is %.0f degrees\n",
         t, current_facing_angle.ToDegrees());
}
~~~

# Creating Your Own Animation Algorithms   {#pie_noon_guide_own_algorithms}

An animation algorithm is a formula that describes the motion of a variable.
Each animation algorithm is processed by an `ImpelProcessor`.

An `ImpelProcessor` holds the data for *every* `Impeller` that uses its
algorithm. Once per frame, `ImpelProcessor::AdvanceFrame` is called and
*all* `Impeller`s are updated in bulk. This bulk storage and processing allows
us to optimize using SIMD and multiple threads, which is important for
scalability.

You can add your own custom animation algorithms by deriving classes from
`ImpelProcessor`. To create a class that animates a one-dimensional variable,
derive from `ImpelProcessor<float>`. For a two-dimensional variable,
derive from `ImpelProcessor<mathfu::vec2>`, etc.

For example, here is a simple one-dimensional algorithm that moves linearly
from the current value to the target value, in the target time.

The header file,

~~~{.cpp}
struct LinearImpelInit {
  IMPEL_INIT_REGISTER();
};

class LinearImpelProcessor : public ImpelProcessor<float> {
  struct ImpelData {
    float value;
    float target_value;
    float target_time;
  };

 public:
  IMPEL_PROCESSOR_REGISTER(LinearImpelProcessor, LinearImpelInit);

  virtual ~LinearImpelProcessor() {
    assert(map_.Count() == 0);
  }

  // When an Impeller is initialized with LinearImpelInit, this function will
  // be called. We allocate data for the new impeller.
  virtual ImpelId InitializeImpeller(const ImpelInit& init,
                                     ImpelEngine* engine) {
    (void)engine;
    assert(init.type == LinearImpelInit::kType);

    // Allocate an external id, and map it to an index into data_.
    ImpelId id = map_.Allocate();

    // Initialize the newly allocated item in data_.
    Data(id).Initialize(static_cast<const LinearImpelInit&>(init));
    return id;
  }

  // When an Impeller is destroyed, or reinitialized, this function will
  // be called. We free the data we had allocated for it.
  virtual void RemoveImpeller(ImpelId id) { map_.Free(id); }

  virtual ImpellerType Type() const { return LinearImpelInit::kType; }

  // Once per frame, the ImpelEngine calls this function. In this one call,
  // we advance the simulation of _all_ linear impellers. Bulk processing
  // allows us to use SIMD or multi-threading when appropriate. Here, the
  // math is (ridiculously) simple, so there's no need.
  virtual void AdvanceFrame(ImpelTime delta_time) {
    // We could optimize this using SIMD.
    for (ImpelData* d = map_.Begin(); d < map_.End(); ++d) {
      if (d->target_time > 0.0f) {
        const float percent_complete = delta_time / d->target_time;
        d->value = mathfu::Lerp(d->value, d->target_value, percent_complete);
      } else {
        d->value = d->target_value;
      }
      d->target_time -= delta_time;
    }
  }

  // Accessors to allow the user to get and set simluation values.
  virtual float Value(ImpelId id) const { return Data(id).value; }
  virtual float TargetValue(ImpelId id) const { return Data(id).target_value; }
  virtual void SetValue(ImpelId id, const float& value) {
    Data(id).value = value;
  }
  virtual void SetTargetValue(ImpelId id, const float& target_value) {
    Data(id).target_value = target_value;
  }
  virtual void SetTargetTime(ImpelId id, float target_time) {
    Data(id).target_time = target_time;
  }
  virtual float Difference(ImpelId id) const {
    const ImpelData& d = Data(id);
    return d.target_value - d.value;
  }

 protected:
  ImpelData& Data(ImpelId id) { return map_.Data(id); }
  const ImpelData& Data(ImpelId id) const { return map_.Data(id); }

  IdMap<ImpelData> map_;
};

~~~

The `.cpp` file,

~~~{.cpp}

IMPEL_INIT_INSTANTIATE(LinearImpelProcessor);

~~~

And to create an `Impeller` that uses our new linear algorithm, first you need
to register your `ImpelProcessor` with the system. In your initialization code,

~~~{.cpp}
impel::LinearImpelProcessor::Register();
~~~

Then you can make any one-dimensional impeller use your new animation algorithm
by simply initializing it with LinearImpelInit.

~~~{.cpp}
impel::Impeller1f impeller;
LinearImpelInit init;
impeller.Initialize(init, engine);
impeller.SetValue(start_value);
impeller.SetTargetValue(target_value);
impeller.SetTargetTime(target_time);
~~~


# Using Your Own Math Types   {#pie_noon_guide_using_your_own_math_types}

Internally, the Impeller system uses mathfu types. All the data in an
`ImpelProcessor` is transmitted using `mathfu::vec2`, `mathfu::vec3`, etc.
However, `ImpelProcessor`s are entirely internal.

The external API is encapsulated in `Impeller`, which is templated
so that you can supply your own vector types. The template parameter takes a
class with to static methods: one to convert from the mathfu type to your type,
and another to convert from your type to the mathfu type.

For example, if you have your own native math types that are byte-wise
compatible with the mathfu types, you could use a union to convert between the
two.

~~~{.cpp}
  template<class MathFuT, class NativeT>
  class UnionConverter {
    union Union {
      MathFuT mathfu;
      NativeT native;
    };
  public:
    typedef NativeT ExternalApiType;
    static NativeT To(const MathFuT& x) {
      Union u;
      u.mathfu = x;
      return u.native;
    }
    static MathFuT From(const NativeT& x) {
      Union u;
      u.native = x;
      return u.mathfu;
    }
  };
~~~

Then, in your own header file, define Impellers export values in your native
types like so,

~~~{.cpp}
  typedef Impeller< UnionConverter<mathfu::vec2, NativeVec2> > ImpellerNative2f;
  typedef Impeller< UnionConverter<mathfu::vec3, NativeVec3> > ImpellerNative3f;
  typedef Impeller< UnionConverter<mathfu::vec4, NativeVec3> > ImpellerNative3f;
~~~

At some point in your update loop, you'll advance the simulation of all
impellers by calling,

~~~{.cpp}
  engine.AdvanceFrame(delta_time);
~~~


<br>

  [Pie Noon]: @ref pie_noon_guide_overview
  [Using Your Own Math Types]: @ref pie_noon_guide_using_your_own_math_types
