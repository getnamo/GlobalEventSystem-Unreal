const { inspect, tryLog, uclass } = require('utility/objectUtility.js');

/** 
Wrapper class to enable some passthrough ges binding.
ATM only supports string params and a maximum of 10 bind events
before it overwrites older binds. This is due to the limitation that
UFUNCTIONS have to be defined at design time. Todo: add support for ~100?

Super experimental atm
*/
class GESJsReceiver extends JsOwner.ClassMap['GESJsReceiverBpActor']{
	ctor(){
		this.callbacks = {};
	}
	constructor(){
	}

	OnJsReceive(uniqueId, property){
		this.callbacks[uniqueId](property);
	}

	bind(domain='global.default', event, callback){
		//const uniqueKey = domain + '.' + event;
		const uniqueFunctionId = this.NextUniqueReceiver()['NextUniqueFunction'];

		if(uniqueFunctionId == 'OnJsReceiveOneParam_9'){
			console.warn('GESJsReceiver: Maximum receivers reached!');
		}

		this.callbacks[uniqueFunctionId] = callback;

		this.JsGESBindEvent(domain,
			event,
			uniqueFunctionId);
	}
	emit(domain='global.default', event, data='', pinned=false){
		if(typeof data === 'string'){
			this.JsGESEmitEventOneParamString(domain, event, data, pinned);
		}
		else{
			this.JsGESEmitEventOneParamObject(domain, event, data, pinned);
		}
	}
	unbindAll(){
		GlobalEventSystemBPLibrary.GESUnbindAllEventsForContext(this);
	}
}

const GESJsReceiver_C = uclass(GESJsReceiver);

exports.ges = new GESJsReceiver_C(GWorld, {Z:0});