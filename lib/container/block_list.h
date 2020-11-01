#ifndef _BLOCK_LIST_H_
#define _BLOCK_LIST_H_

#include <misc/cpp_attributes.h>
#include <optimized/bits.h>

namespace container {

template<typename T>
struct node_block;

template<typename T>
struct node {
    T              obj;
    const uint32_t idx;

    node(T && _obj, const uint32_t _idx) : obj(_obj), idx(_idx) {}

    ALWAYS_INLINE CONST_ATTR node_block<T> *
                             get_block() const {
        return (node_block<T> *)(((uint64_t)this) -
                                 (40 + idx * sizeof(node<T>)));
    }

    uint64_t ALWAYS_INLINE
    drop_mask() const {
        return (1UL) << idx;
    }

    void ALWAYS_INLINE
    destroy() {
        if constexpr (!(std::is_trivially_destructible<T>::value)) {
            obj.~T();
        }
    }
};

template<typename T>
struct node_block {
    using node_t = node<T>;

    uint64_t node_slots;

    node_block<T> * next_iter;
    node_block<T> * prev_iter;

    node_block<T> * next_available;
    node_block<T> * prev_available;

    node_t nodes[64];

    uint64_t
    insert(T && obj) {
        const uint32_t idx = bits::find_first_one<uint64_t>(node_slots);
        node_slots &= (node_slots - 1);
        new ((void * const)(nodes + idx)) node(std::forward<T>(obj), idx);
        return ((uint64_t)(nodes + idx)) | (!node_slots);
    }


    void
    init() {
        node_slots = ~(1L);
    }


    void
    destroy_batch(const uint64_t mask) {
        if constexpr (!(std::is_trivially_destructible<T>::value)) {
            uint64_t _node_slots = node_slots;
            _node_slots          = (~_node_slots) & mask;
            do {
                const uint32_t idx =
                    bits::find_first_one<uint64_t>(_node_slots);
                _node_slots &= (_node_slots - 1);
                (nodes + idx)->destroy();
            } while (_node_slots);
        }
    }

    void
    destroy() {
        if constexpr (!(std::is_trivially_destructible<T>::value)) {
            uint64_t _node_slots = node_slots;
            _node_slots          = ~_node_slots;
            do {
                const uint32_t idx =
                    bits::find_first_one<uint64_t>(_node_slots);
                _node_slots &= (_node_slots - 1);
                (nodes + idx)->destroy();
            } while (_node_slots);
        }
    }

    uint64_t ALWAYS_INLINE
    drop(const uint64_t mask) {
        node_slots |= mask;
        return node_slots;
    }

    uint64_t ALWAYS_INLINE
    drop(node_t * n) {
        return drop(n->idx);
    }

    void
    show() {
        fprintf(stderr,
                "[%p] -> { 0x%016lx | %p -> | <- %p | %p -> | <- %p}\n",
                this,
                node_slots,
                next_iter,
                prev_iter,
                next_available,
                prev_available);
    }
};

template<typename T>
struct node_iterator;

template<typename T>
struct block_iterator;

template<typename T>
struct single_node_iterator;

template<typename T>
struct block_list {
    using node_iterator_t        = node_iterator<T>;
    using single_node_iterator_t = single_node_iterator<T>;
    using block_iterator_t       = block_iterator<T>;
    using node_block_t           = node_block<T>;
    using node_t                 = typename node_block_t::node_t;

    node_block_t * iter_list;
    node_block_t * available_list;
    block_list() = default;
    ~block_list() {
        if (iter_list != NULL) {
            destroy();
        }
    }

    void
    zero() {
        iter_list      = NULL;
        available_list = NULL;
    }

    void
    link_available(node_block_t * const new_block) {
        new_block->next_available = available_list;
        if (available_list != NULL) {
            available_list->prev_available = new_block;
        }
        available_list = new_block;
    }

    void
    unlink_available(node_block_t * const new_block) {
        node_block_t * const next_block = new_block->next_available;
        node_block_t * const prev_block = new_block->prev_available;
        if (next_block != NULL) {
            next_block->prev_available = prev_block;
        }
        if (prev_block != NULL) {
            prev_block->next_available = next_block;
        }
        else {
            available_list = next_block;
        }
    }

    void
    link_block(node_block_t * const new_block) {
        new_block->next_iter = iter_list;
        if (iter_list != NULL) {
            iter_list->prev_iter = new_block;
        }
        iter_list = new_block;
    }

    void
    unlink_block(node_block_t * const new_block) {
        node_block_t * const next_block = new_block->next_iter;
        node_block_t * const prev_block = new_block->prev_iter;
        if (next_block != NULL) {
            next_block->prev_iter = prev_block;
        }
        if (prev_block != NULL) {
            prev_block->next_iter = next_block;
        }
        else {
            iter_list = next_block;
        }
    }


    T *
    _insert(T && obj) {
        if (BRANCH_UNLIKELY(available_list == NULL)) {
            node_block_t * const new_block =
                (node_block_t * const)calloc(1, sizeof(node_block_t));

            new_block->init();
            new ((void * const)(new_block->nodes))
                node(std::forward<T>(obj), 0);


            link_block(new_block);
            link_available(new_block);

            return (T *)(new_block->nodes);
        }
        else if (iter_list == NULL) {
            node_block_t * const block = available_list;
            assert(block->node_slots == (~(0UL)));
            new ((void * const)(block->nodes)) node(std::forward<T>(obj), 0);
            block->init();
            iter_list = block;
            block->next_iter = NULL;
            block->prev_iter = NULL;
            return (T *)(block->nodes);
        }
        else {
            node_block_t * const block = available_list;
            uint64_t ret = block->insert(std::move(obj));
            if (BRANCH_UNLIKELY(ret & 0x1)) {
                available_list = available_list->next_available;
                if (available_list != NULL) {
                    available_list->prev_available = NULL;
                }
                ret ^= 0x1;
            }
            return (T *)ret;
        }
    }


    ALWAYS_INLINE T *
                  insert(const T & obj) {
        return _insert(std::move(obj));
    }

    ALWAYS_INLINE T *
                  insert(T && obj) {
        return _insert(std::move(obj));
    }


    T *
    head() {
        node_block_t * const block       = iter_list;
        const uint64_t       _node_slots = ~(block->node_slots);
        node_t * const       n =
            block->nodes + bits::find_first_one<uint64_t>(_node_slots);
        return &(n->obj);
    }

    T *
    pop() {
        node_block_t * const block = iter_list;
        uint64_t _node_slots = ~(block->node_slots);

        node_t * const n =
            block->nodes + bits::find_first_one<uint64_t>(_node_slots);

        // was full
        if (BRANCH_UNLIKELY(_node_slots == (~(0UL)))) {
            link_available(block);
        }

        _node_slots &= (_node_slots - 1);

        // is empty
        if (_node_slots == 0) {
            unlink_block(block);
            // if we are only free block leave
            if (available_list->next_available != NULL) {
                unlink_available(block);
                free(block);
                return &(n->obj);
            }
        }
        block->node_slots = ~_node_slots;
        return &(n->obj);
    }

    void
    remove_batch(node_block_t * const block, const uint64_t mask) {
        block->destroy_batch(mask);
        const uint64_t ret = block->drop(mask);
        if (BRANCH_UNLIKELY(ret == mask)) {
            link_available(block);
        }
        else if (ret == (~(0UL))) {
            unlink_block(block);
            // if we are only free block leave
            if (available_list->next_available != NULL) {
                unlink_available(block);
                free(block);
            }
        }
    }

    void
    remove(T * t) {
        remove((node_t *)t);
    }

    void
    remove(node_t * n) {
        n->destroy();
        node_block_t * const block = n->get_block();

        const uint64_t drop_mask = n->drop_mask();
        const uint64_t ret       = block->drop(drop_mask);
        if (BRANCH_UNLIKELY(ret == drop_mask)) {
            link_available(block);
        }
        else if (BRANCH_UNLIKELY(ret == (~(0UL)))) {
            unlink_block(block);
            // if we are only free block leave
            if (available_list->next_available != NULL) {
                unlink_available(block);
                free(block);
            }
        }
    }


    void
    destroy() {
        node_block_t * tmp = available_list;
        while(tmp) {
            tmp->destroy();
            tmp = tmp->next_iter;
        } 
    }

    void
    check() {
        node_block_t * tmp   = available_list;
        uint32_t       count = 0;

        while (tmp) {
            assert(tmp->node_slots);
            ++count;

            if (tmp->next_available) {
                assert(tmp->next_available->prev_available == tmp);
            }
            tmp = tmp->next_available;
        }

        tmp = iter_list;
        while (tmp) {
            if (tmp->node_slots) {
                --count;
            }
            if (tmp->next_iter) {
                assert(tmp->next_iter->prev_iter == tmp);
            }
            tmp = tmp->next_iter;
        }
        assert(count == 0 || count == 1);
    }

    void
    show_iter_list() {
        node_block_t * tmp = iter_list;
        while (tmp) {
            tmp->show();
            tmp = tmp->next_iter;
        }
    }

    void
    show_available_list() {
        node_block_t * tmp = available_list;
        while (tmp) {
            tmp->show();
            tmp = tmp->next_available;
        }
    }

    node_iterator_t
    nbegin() const {
        return node_iterator_t(iter_list);
    }

    block_iterator_t
    bbegin() const {
        return block_iterator_t(iter_list);
    }
};


template<typename T>
struct node_iterator {
    using node_block_t = typename block_list<T>::node_block_t;

    node_block_t * block;
    uint64_t       cur_node_slots;

    node_iterator() : block(NULL) {}

    node_iterator(node_block_t * _block) {
        block          = _block;
        cur_node_slots = (_block == NULL) ? 0 : (~(_block->node_slots));
    }

    ALWAYS_INLINE T *
                  get() const {
        return (T *)(block->nodes +
                     bits::find_first_one<uint64_t>(cur_node_slots));
    }

    uint32_t ALWAYS_INLINE
    end() const {
        return block == NULL;
    }

    void
    next() {
        cur_node_slots &= (cur_node_slots - 1);
        if (!cur_node_slots) {
            block = block->next_iter;
            if (block) {
                cur_node_slots = ~(block->node_slots);
            }
        }
    }

    void ALWAYS_INLINE
    operator++() {
        next();
    }

    void ALWAYS_INLINE
    operator++(int) {
        next();
    }
};


template<typename T>
struct single_node_iterator {
    using node_block_t = typename block_list<T>::node_block_t;
    using node_t       = typename block_list<T>::node_t;


    node_t * const nodes;
    uint64_t       cur_node_slots;

    single_node_iterator() : cur_node_slots(0) {}

    single_node_iterator(node_block_t * _block) : nodes(_block->nodes) {
        cur_node_slots = (_block == NULL) ? 0 : (~(_block->node_slots));
    }

    ALWAYS_INLINE T *
                  get() const {
        return (T *)(nodes + bits::find_first_one<uint64_t>(cur_node_slots));
    }

    uint32_t ALWAYS_INLINE
    end() const {
        return cur_node_slots == 0;
    }

    void
    next() {
        cur_node_slots &= (cur_node_slots - 1);
    }

    void ALWAYS_INLINE
    operator++() {
        next();
    }

    void ALWAYS_INLINE
    operator++(int) {
        next();
    }
};


template<typename T>
struct block_iterator {
    using node_block_t = typename block_list<T>::node_block_t;
    using single_node_iterator_t =
        typename block_list<T>::single_node_iterator_t;
    node_block_t * block;

    block_iterator() : block(NULL) {}

    block_iterator(node_block_t * _block) {
        block = _block;
    }

    node_block_t *
    get() {
        return block;
    }

    uint32_t ALWAYS_INLINE
    end() const {
        return block == NULL;
    }

    void
    next() {
        block = block->next_iter;
    }

    void ALWAYS_INLINE
    operator++() {
        next();
    }

    void ALWAYS_INLINE
    operator++(int) {
        next();
    }

    single_node_iterator_t
    nbegin() const {
        return single_node_iterator_t(block);
    }
};

}  // namespace container

#endif
