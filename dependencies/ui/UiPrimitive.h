#pragma once

#include "W4Framework.h"
#include "common/Updatable.h"

W4_USE_UNSTRICT_INTERFACE

class UiPrimitive : public Updatable
{
#ifdef DEBUG
    static inline int primitiveCounter = 0;
#endif

public:
    UiPrimitive(cref<Node> root) : m_root(make::sptr<Node>())
    {
        m_root->setParent(root, false);

#ifdef DEBUG
        m_root->setName(std::string("ui_primitive_root") + "##" + std::to_string(primitiveCounter++));
#endif
    }

    virtual ~UiPrimitive()
    {
        m_root->setParent(nullptr);
    }

    void update(float dt) override {}
    [[nodiscard]] virtual bool isUpdatable() = 0;

    virtual void setEnabled(bool flag = true)
    {
        m_root->setEnabled(flag);
    }
    [[nodiscard]] virtual bool isEnabled() const
    {
        return m_root->isEnabled();
    }
    [[nodiscard]] bool isEnabledByWindow() const
    {
        return m_enabledByWindow;
    }
    void setEnabledByWindow(bool flag)
    {
        m_enabledByWindow = flag;
    }

    virtual void onResize(uvec2 const& sz) {}

    [[nodiscard]] cref<Node> getRoot() const
    {
        return m_root;
    }
    [[nodiscard]] sptr<Node> getRoot()
    {
        return m_root;
    }

protected:
    sptr<Node> m_root;
    bool m_enabledByWindow = true;
};
