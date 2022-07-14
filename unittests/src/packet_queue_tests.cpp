#include "common.h"

#include "../../gui/stdafx.h"
#include "../../core/priority.h"
#include "../../core/connections/wim/queue/packet_queue.h"

struct queue_element
{
    int key_;
};

struct key_getter
{
    int key(const queue_element& _element) const
    {
        return _element.key_;
    }

    core::priority_t priority(const queue_element& _element) const
    {
        return _element.key_;
    }
};

using test_queue = core::wim::packet_queue<queue_element, key_getter>;

TEST(packet_queue_test, can_pop_element)
{
    { // pop single element
        test_queue queue;
        queue.set_limit(10);
        EXPECT_FALSE(queue.can_pop_element());
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());
        auto e = queue.pop();
        EXPECT_FALSE(queue.can_pop_element());
    }
    { // pop dequeued element
        test_queue queue;
        queue.set_limit(10);
        EXPECT_FALSE(queue.can_pop_element());
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());
        auto e = queue.pop();
        queue.enqueue({ 1 });
        EXPECT_FALSE(queue.can_pop_element());
    }
    { // pop allowed element
        test_queue queue;
        queue.set_limit(10);
        EXPECT_FALSE(queue.can_pop_element());
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());

        auto e = queue.pop();
        queue.allow_using(1);

        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());
    }
    { // pop multiple element
        test_queue queue;
        queue.set_limit(10);
        EXPECT_FALSE(queue.can_pop_element());
        constexpr auto max_elements = 4;
        for (auto i = 0; i < max_elements; ++i)
            queue.enqueue({ i });

        for (auto i = 0; i < max_elements; ++i)
        {
            EXPECT_TRUE(queue.can_pop_element());
            auto e = queue.pop();
        }
        EXPECT_FALSE(queue.can_pop_element());
    }
    { // enqueue extracted element
        test_queue queue;
        queue.set_limit(10);
        constexpr auto max_elements = 4;
        for (auto i = 0; i < max_elements; ++i)
            queue.enqueue({ i });

        auto e0 = queue.pop();
        auto e1 = queue.pop();
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());

        queue.allow_using(1);
        EXPECT_TRUE(queue.can_pop_element());
    }
    { // enqueue extracted element & pop it
        test_queue queue;
        queue.set_limit(10);
        constexpr auto max_elements = 4;
        for (auto i = 0; i < max_elements; ++i)
            queue.enqueue({ i });

        auto e0 = queue.pop();
        auto e1 = queue.pop();
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());

        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);

        queue.allow_using(1);
        EXPECT_TRUE(queue.can_pop_element());

        e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
    }
}

TEST(packet_queue_test, check_limit)
{
    { // check defautl limit
        test_queue queue;
        EXPECT_FALSE(queue.can_pop_element());
        EXPECT_FALSE(queue.is_limit_reached());
        queue.enqueue({ 1 });
        EXPECT_TRUE(queue.can_pop_element());
        EXPECT_FALSE(queue.is_limit_reached());
        auto e = queue.pop();
        EXPECT_FALSE(queue.can_pop_element());
        EXPECT_TRUE(queue.is_limit_reached());
    }
    { // check set limit
        test_queue queue;
        constexpr auto max_elements = 10;
        queue.set_limit(max_elements);
        EXPECT_FALSE(queue.is_limit_reached());
        for (auto i = 0; i < 2 * max_elements; ++i)
            queue.enqueue({ i });

        std::vector<std::shared_ptr<queue_element>> elements;
        elements.reserve(max_elements);
        for (auto i = 0; i < max_elements; ++i)
        {
            EXPECT_TRUE(queue.can_pop_element());
            EXPECT_FALSE(queue.is_limit_reached());
            elements.push_back(queue.pop());
        }

        EXPECT_FALSE(queue.can_pop_element());
        EXPECT_TRUE(queue.is_limit_reached());
    }
    { // check zero limit
        test_queue queue;
        queue.set_limit(0);
        queue.enqueue({ 1 });
        EXPECT_FALSE(queue.can_pop_element());
        EXPECT_TRUE(queue.is_limit_reached());
        queue.set_limit(1);
        EXPECT_TRUE(queue.can_pop_element());
        EXPECT_FALSE(queue.is_limit_reached());
        queue.set_limit(0);
        EXPECT_FALSE(queue.can_pop_element());
        EXPECT_TRUE(queue.is_limit_reached());
    }
    { // set increased limit
        test_queue queue;
        queue.set_limit(1);
        EXPECT_FALSE(queue.is_limit_reached());
        queue.enqueue({ 1 });
        EXPECT_FALSE(queue.is_limit_reached());
        auto e1 = queue.pop();
        EXPECT_TRUE(queue.is_limit_reached());
        queue.enqueue({ 2 });
        queue.set_limit(2);
        EXPECT_FALSE(queue.is_limit_reached());
        auto e2 = queue.pop();
        EXPECT_TRUE(queue.is_limit_reached());
    }
    { // set decreased limit
        test_queue queue;
        queue.set_limit(3);
        queue.enqueue({ 1 });
        queue.enqueue({ 2 });
        queue.enqueue({ 3 });
        auto e1 = queue.pop();
        auto e2 = queue.pop();
        auto e3 = queue.pop();
        EXPECT_TRUE(queue.is_limit_reached());
        queue.set_limit(2);
        EXPECT_TRUE(queue.is_limit_reached());
        queue.allow_using(1);
        EXPECT_TRUE(queue.is_limit_reached());
        queue.allow_using(2);
        EXPECT_FALSE(queue.is_limit_reached());
        queue.set_limit(1);
        EXPECT_TRUE(queue.is_limit_reached());
        queue.allow_using(3);
        EXPECT_FALSE(queue.is_limit_reached());
    }
}

TEST(packet_queue_test, release_elements)
{
    { // check releasing used elements by release
        test_queue queue;
        queue.set_limit(2);
        queue.enqueue({ 1 });
        queue.enqueue({ 1 });
        auto e = queue.pop();
        EXPECT_FALSE(queue.can_pop_element());
        queue.allow_using(1);
        EXPECT_TRUE(queue.can_pop_element());
    }
}


struct queue_element_with_priority
{
    int key_;
    int priority_;
};

struct priority_key_getter
{
    int key(const queue_element_with_priority& _element) const
    {
        return _element.key_;
    }
    core::priority_t priority(const queue_element_with_priority& _element) const
    {
        return _element.priority_;
    }
};

using priority_test_queue = core::wim::packet_queue<queue_element_with_priority, priority_key_getter>;

TEST(packet_queue_test, pop_top_element)
{
    { // sort by insert time - FIFO
        priority_test_queue queue;
        queue.enqueue({ 1, 1 });
        queue.enqueue({ 2, 1 });
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
    }
    { // sort by insert time - FIFO
        priority_test_queue queue;
        queue.enqueue({ 2, 1 });
        queue.enqueue({ 1, 1 });
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
    }
    { // sort by insert time - FIFO - equal priority at end
        priority_test_queue queue;
        queue.enqueue({ 0, 0 });
        queue.enqueue({ 1, 1 });
        queue.enqueue({ 2, 1 });
        auto e0 = queue.pop();
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
    }
    { // sort by insert time - FIFO - equal priority at end
        priority_test_queue queue;
        queue.enqueue({ 0, 2 });
        queue.enqueue({ 1, 1 });
        queue.enqueue({ 2, 1 });
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
        auto e0 = queue.pop();
        ASSERT_EQ(e0->key_, 0);
    }
    { // sort by insert time - FIFO - equal priority at front
        priority_test_queue queue;
        queue.enqueue({ 1, 1 });
        queue.enqueue({ 2, 1 });
        queue.enqueue({ 0, 0 });
        auto e0 = queue.pop();
        ASSERT_EQ(e0->key_, 0);
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
    }
    { // sort by insert time - FIFO - equal priority at front
        priority_test_queue queue;
        queue.enqueue({ 1, 1 });
        queue.enqueue({ 2, 1 });
        queue.enqueue({ 0, 2 });
        auto e1 = queue.pop();
        ASSERT_EQ(e1->key_, 1);
        auto e2 = queue.pop();
        ASSERT_EQ(e2->key_, 2);
        auto e0 = queue.pop();
        ASSERT_EQ(e0->key_, 0);
    }
}
