#include "Object.h"

namespace Jimara {
	Object::Object() : m_referenceCount(1) { }

	Object::~Object() { }

	void Object::AddRef()const { m_referenceCount++; }

	void Object::ReleaseRef()const {
		std::size_t count = m_referenceCount.fetch_sub(1);
		if (count == 1) OnOutOfScope();
	}

	void Object::OnOutOfScope()const {
		delete this;
	}
}
