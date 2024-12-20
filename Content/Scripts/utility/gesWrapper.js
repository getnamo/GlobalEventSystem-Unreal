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

	OnJsReceiveObj(uniqueId, property){
		this.callbacks[uniqueId](property);
	}

	//returns a unique id for potential unbinding
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
		return uniqueFunctionId;
	}

	bindToObjCallback(domain='global.default', event, callback, uniqueReceiver = false){
		//const uniqueKey = domain + '.' + event;
		const uniqueFunctionId = this.NextUniqueReceiverObj()['NextUniqueFunction'];

		if(uniqueReceiver && uniqueFunctionId == 'OnJsReceiveOneParamObj_1'){
			console.warn('GESJsReceiver: Maximum obj receivers (1) reached (uniqueReceiver: true)!');
			return '';
		}
		
		if(uniqueFunctionId == 'OnJsReceiveOneParamObj_4'){
			console.warn('GESJsReceiver: Maximum obj receivers reached!');
		}

		this.callbacks[uniqueFunctionId] = callback;

		console.log(`UniqueId is <${uniqueFunctionId}>`)

		this.JsGESBindEvent(domain,
			event,
			uniqueFunctionId);
		return uniqueFunctionId;
	}

	emit(domain='global.default', event, data='', pinned=false){
		if(typeof data === 'string'){
			this.JsGESEmitEventOneParamString(domain, event, data, pinned);
		}
		else{
			this.JsGESEmitEventOneParamObject(domain, event, data, pinned);
		}
	}

	//NB: need to store the unique function id you get from bind
	unbind(domain='global.default', event, uniqueFunctionId){
		this.UnbindEvent(domain, event, uniqueFunctionId);
	}

	wlog(text){
		if(typeof text !== 'string'){
			text = JSON.stringify(text);
		}
		this.emit('global.console', 'log', text);
	}
	unbindAll(){
		this.UnbindAllEvents();
		//GlobalEventSystemBPLibrary.GESUnbindAllEventsForContext(this);
	}
}

const GESJsReceiver_C = uclass(GESJsReceiver);

exports.ges = new GESJsReceiver_C(GWorld, {Z:0});