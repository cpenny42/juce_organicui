/*
==============================================================================

Expression.h
Created: 20 Feb 2017 5:01:11pm
Author:  Ben

==============================================================================
*/

#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

class ScriptExpression :
	public Timer,
	public EngineListener,
	public ScriptTarget::ScriptTargetListener,
	public Inspectable::InspectableListener,
	public ParameterListener
{
public:
	ScriptExpression();
	~ScriptExpression();

	enum ExpressionState { EXPRESSION_LOADED, EXPRESSION_ERROR, EXPRESSION_EMPTY, EXPRESSION_CLEAR };

	ExpressionState state;
	ScopedPointer<JavascriptEngine> scriptEngine;

	String expression;
	var currentValue;

	Array<WeakReference<Parameter>> linkedParameters;

	void setExpression(const String &expression);
	void evaluate(bool resetListeners = false);
	void buildEnvironment();
	void setState(ExpressionState newState);

	void scriptObjectUpdated(ScriptTarget *) override;

	void endLoadFile() override;

	Array<Parameter *> getParameterReferencesInExpression();

	void inspectableDestroyed(Inspectable * i) override;
	void parameterValueChanged(Parameter * p) override;

	ListenerList<ExpressionListener> expressionListeners;
	void addExpressionListener(ExpressionListener* newListener) { expressionListeners.add(newListener); }
	void removeExpressionListener(ExpressionListener* listener) { expressionListeners.remove(listener); }


	// Inherited via Timer
	virtual void timerCallback() override;
};

#endif  // EXPRESSION_H_INCLUDED
