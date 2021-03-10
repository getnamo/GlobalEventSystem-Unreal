# global-event-system-ue4
A loosely coupled internal global event system (GES) plugin for the Unreal Engine. Aims to solve cross-map and cross-blueprint communication for reliable and inferable event flow. Should enable a publisher-observer pattern.

[![GitHub release](https://img.shields.io/github/release/getnamo/global-event-system-ue4.svg)](https://github.com/getnamo/global-event-system-ue4/releases)
[![Github All Releases](https://img.shields.io/github/downloads/getnamo/global-event-system-ue4/total.svg)](https://github.com/getnamo/global-event-system-ue4/releases)

Because the events are emitted to a dynamic map of listeners you can loosely link parts of your project without needing to redo boilerplate when you change parts of the code, dynamically change environments, or e.g. load a different submap. Fire something away, and if something is interested in that information, they can do something with it; optional.

Questions? See https://github.com/getnamo/global-event-system-ue4/issues

Discussions? See [Unreal Thread](https://forums.unrealengine.com/development-discussion/engine-source-github/1691290-plugin-global-event-system)

## Quick Install & Setup ##
 1. [Download Latest Release](https://github.com/getnamo/global-event-system-ue4/releases)
 2. Create new or choose project.
 3. Browse to your project folder (typically found at Documents/Unreal Project/{Your Project Root})
 4. Copy *Plugins* folder into your Project root.
 5. Plugin should be now ready to use.

## How to use - Basics and API

There are globally available functions that you can use to emit and bind events. At this time there are two variants for emitting (no parameters and one wildcard parameter) and one for binding events to your local functions. 

Each emit is a multi-cast to all valid bound receivers. If the parameters don't match you'll be warned in the log with fairly verbose messages while emitting to all other valid targets. 

Consider optionally using and extending GESReceiver components or attaching interfaces to your receiving function owners to keep messaging organized.

### Emit Event

#### ```GESEmitEvent```

##### Param: Pinned
Whether the event should trigger for listeners added after the event has fired. Useful to communicate state.

##### Param: Domain
A string type similar to a namespace with reverse-DNS like structure encouraged, but not enforced. By default there is a ```global.default``` prefilled which can be changed to any valid utf8 string.

##### Param: Event
This is an abstract name and is considered unique for that domain. You'll need the same domain and event name to match in your binding function to receive the event.

#### ```GESEmitEventOneParam```

##### Additional Param: Parameter Data
Wildcard Property, will accept any single property type e.g. *int, float, byte, string, name, bool, struct,* and *object*. Wrap arrays and maps in a custom struct to emit more complex data types. 

Break pin to set a new type of value. 

Keep in mind that the receiving listeners need to match the property type to receive the data.

![emit](https://i.imgur.com/8nXb5ya.png)

### Bind Event

```GESBindEvent```
##### Param: Domain
A string type similar to a namespace with reverse-DNS like structure encouraged, but not enforced. By default there is a ```global.default``` prefilled which can be changed to any valid utf8 string.

##### Param: Event
This is an abstract name and is considered unique for that domain. You'll need the same domain and event name to match in your emitting function to receive the event.

##### Param: Receiving Function
The final parameter is your local function name. This will be called on the graph context object (owner of graph e.g. the calling actor).

![bind](https://i.imgur.com/WzHhEeG.png)

Then make your custom event or blueprint function with a matching name and matching parameters.

### Bind Event to Wildcard Delegate

Instead of linking via function name, you can connect or make a wildcard property delegate (c++ type _FGESOnePropertySignature_).

From that event you can try to convert your wildcard parameter into the types you want to receiver with a boolean specifying if the conversion was successful.

### Options

There are some simple options to toggle some log messages and detailed struct type checking.

![options](https://i.imgur.com/22tC4lI.png)

## Examples

Keep in mind that you can start using GES incrementally for specific tasks or parts of large projects instead of replacing too many parts at once. Below are some very basic examples where GES could be useful.

### Cross-map reference pinning
Let's say you had two actors in two different sub-maps and you wanted one actor to know that it has spawned from e.g. some dynamic process. Delay nodes shown below are only used to show example event delays due to e.g. async processing or waiting on something else to happend; not needed for function.

![actor ready](https://i.imgur.com/BLUFoFs.png)
In the spawned actor you could emit a reference to itself.

![listen actor](https://i.imgur.com/IP0XTtC.png)

and in the other actor you could bind to that event to do something with that information. Normally even without pinning this event should be received because you bind before you emit. But what if you couldn't control the delay?

![delayed bind](https://i.imgur.com/UfQYsJa.png)

This is the case where pinning the event would help as now when the receiving actor binds to the event, it will automatically receive the last emit even though it was called after the event was emitted. From a developer perspective you can now just handle the receiving logic and not worry about whether you need to add delays or loop through all actors in the map. By arranging your events to signal selectively states and muxing those states you can ensure that the order of your events remains predictable; only start x when part y and z in the map have happened.

### Flow muxing and loose coupling

You can add a simple actor to the map which listens to various GES events. When for example two of those events have fired you can fire off another event which is a composite logic of the source events e.g. ANDGate or much more complex logic if we decide to use variable state.

![](https://i.imgur.com/ickckJe.png)

Blueprints which would listen to the SAReady event, don't even have to care where the source came from and you could easily swap out this logic actor for maybe another type without changing any other code; an example of the loose coupling enabled by GES. The actor is replaceable, there is no additional boilerplate that needs to be changed if replaced.

## C++

To use GES in C++, add ```"GlobalEventSystem"``` to your project Build.cs e.g.

```PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GlobalEventSystem" });```

then in your implementation file of choice add the ```#include "GESHandler.h"``` header.

### Emit an event

Use _FGESHandler_ class to get static access to a default handler.

```FGESHandler::DefaultHandler()```

Call functions on this handler to both emit and bind events.

#### No param
To emit a no-param event you specify an _FGESEmitContext_ struct as the first function parameter

```c++
//define emit contexts
FGESEmitContext Context;
Context.Domain = TEXT("global.default");
Context.Event = TEXT("MyEvent");
Context.bPinned = true;      //whether the event state should be available after emit
Context.WorldContext = this; //all GES events require a WorldContext object, typically this will be an actor or anything with a world.

FGESHandler::DefaultHandler()->EmitEvent(Context);
```

#### One param

For any other emit type with one parameter, you pass the parameter value of choice as the second function parameter.
Most common types are overloaded in _EmitEvent_. For multi-param and complex data types wrap or use a UStruct or UObject sub-class.

##### FString

```c++
...

FString MyString = TEXT("MyStringData");
FGESHandler::DefaultHandler()->EmitEvent(Context, MyString);
```

or you can emit string literals via

```c++
...
FGESHandler::DefaultHandler()->EmitEvent(Context, TEXT("MyStringData"));
```

##### int32
```c++
...

FGESHandler::DefaultHandler()->EmitEvent(Context, 5);
```

##### float
```c++
...

FGESHandler::DefaultHandler()->EmitEvent(Context, 1.3);
```

##### bool

```c++
...

FGESHandler::DefaultHandler()->EmitEvent(Context, true);
```

##### FName

```c++
...

FName MyName = TEXT("my name");
FGESHandler::DefaultHandler()->EmitEvent(Context, MyName);
```

##### UObject*
```c++
...

UObject* SomeObject;

FGESHandler::DefaultHandler()->EmitEvent(Context, SomeObject);
```

##### Struct

```c++

//Assuming e.g. this custom struct definition
//NB : blueprint type declaration is optional, but will expose it to bp for easier receiving in that context
USTRUCT(BlueprintType)
struct FCustomTestData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category= Test)
    FString Name;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Test)
    int32 Index;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Test)
    TArray<float> Data;
};

...

FCustomTestData EmitStruct;
EmitStruct.Name = TEXT("Testy");
EmitStruct.Index = 5;
EmitStruct.Data = {1.2, 2.3};


FGESHandler::DefaultHandler()->EmitEvent(FCustomTestData::StaticStruct(), &EmitStruct);
```

### Receive an event

The recommended method is using lambda receivers. Define an _FGESEventContext_ struct as the first param, then pass your overloaded lambda as the second type. NB: you can also alternatively organize your receivers with e.g. subclassing a _GESBaseReceiverComponent_, but these are only applicable for actor owners and thus not recommended over lambda receivers in general. 

#### No param event

Only the bind info is required. Use 'this' capture context in the lambda to enable calling e.g. member functions.

```c++
FGESEventContext Context;
Context.Domain = TEXT("global.default");
Context.Event = TEXT("MyEvent");
Context.WorldContext = this;
 
FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this]
{
    //handle receive
});
```

#### FString param event

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](const FString& StringData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %s"), *StringData);
});
```

#### float param event

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](float FloatData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %1.3f"), FloatData);
});
```

#### int32 param event

NB: name specialization of this bind due to lambda bind ambiguity with float callback

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListenerInt(Context, [this](int32 IntData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %d"), IntData);
});
```

#### bool param event

NB: name specialization of this bind due to lambda bind ambiguity with float callback

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListenerBool(Context, [this](bool BoolData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %d"), BoolData);
});
```

#### FName param event

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](const FName& NameData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %s"), *NameData.ToString());
});
```

#### UObject* param event

```c++
...

FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](UObject* ObjectData)
{
    //handle receive, e.g. log result
    UE_LOG(LogTemp, Log, TEXT("Received %s"), *ObjectData.GetName());
});
```

#### Struct param event

Structs need a deep copy to be readable.

```c++

//Assuming this custom struct
USTRUCT(BlueprintType)
struct FCustomTestData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Test)
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Test)
	int32 Index;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Test)
	TArray<float> Data;
};

...

FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](UStruct* Struct, void* StructPtr)
{
    //Confirm matching struct
    if (Struct == FCustomTestData::StaticStruct())
    {
        //Deep copy your struct to local ref
	FCustomTestData TestData;
	TestData = *(FCustomTestData*)StructPtr;
        
	//Test data is now usable
	UE_LOG(LogTemp, Log, TEXT("Struct data: %s %d"), *TestData.Name, TestData.Data.Num());
    }
});
```

#### Wildcard
If you're not sure of the type of data you can receive, try a wildcard lambda and cast to test validity of data types. You'll need to add ```"#include "GlobalEventSystemBPLibrary.h"``` to use the wildcard property conversion functions.

```c++
...
FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this](const FGESWildcardProperty& WildcardProperty)
{
    //Let's try to decode a float
    float MaybeFloat;
    bool bDidGetFloat = UGlobalEventSystemBPLibrary::Conv_PropToFloat(WildcardProperty, MaybeFloat);
    if(bDidGetFloat)
    {
        //good to go
    }
});
```

#### Unbinding Events
Each bound event function should unbind automatically when the world gets removed, but you can also unbind a lambda via the lambda name return you get when you bind the listener to the event.

```c++
...
 
FString LambdaFunctionName = FGESHandler::DefaultHandler()->AddLambdaListener(Context, [this]
{
    //handle receive
});

...

//let's say we're done listening now
FGESHandler::DefaultHandler()->RemoveLambdaListener(Context, LambdaFunctionName);

```

Optionally you can also store the function and pass it instead of the lambda name to unbind it. Name method is preferred due to developers often defining anonymous functions inline when binding.

## When not to use GES
- There are some performance considerations to keep in mind. While the overall architecture is fairly optimized, it can be more expensive than a simple function call due to function and type checking. Consider it appropriate for signaling more than a hammer to use everywhere.

- If your objects have a tight coupling or it's easily accessible in a tree hierarchy pattern I would use standard methods instead of GES.

- Background threads. Current version is not thread safe and should be called only in your game thread.

## Possible Improvements
See https://github.com/getnamo/global-event-system-ue4/issues for latest.
General enhancements:
- Event with callback (get information from a listener)
- Add optional logging utility to record event flow with possibly replay (attach middleware function)
- Trigger limits, e.g. can only trigger n times
- Add receiver limits (target requires interface/etc)
- Bind to Interface (binds all events in an interface map to functions in interface)
