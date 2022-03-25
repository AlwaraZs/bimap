#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>

template <class T, class tag, class Compare = std::less<T>> struct sorted_tree {
    using type = T;

    struct node {
        T value;
        node *parent{};
        node *left{};
        node *right{};
    };

    struct iterator {
    public:
        iterator(node *_node, node *end) : _node(_node), _end(end) {}

        [[nodiscard]] bool operator==(iterator const &i) const {
            return _node == i._node;
        }

        [[nodiscard]] bool operator!=(iterator const &i) const {
            return !(*this == i);
        }

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        type const &operator*() const { return _node->value; }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        iterator &operator++() {
            if (_node == _end) {
                return *this;
            }
            if (_node->right == _end) {
                while (_node->parent->right == _node) {
                    _node = _node->parent;
                }
                _node = _node->parent;
            } else {
                _node = _node->right;

                while (_node->left != _end) {
                    _node = _node->left;
                }
            }

            return *this;
        }

        iterator operator++(int) {
            iterator ret{*this};
            ++*this;
            return ret;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        iterator &operator--() {
            if (_node->left == _end) {
                while (_node->parent->left == _node) {
                    _node = _node->parent;
                }
                _node = _node->parent;
            } else {
                _node = _node->left;
                while (_node->right != _end) {
                    _node = _node->right;
                }
            }

            return *this;
        }

        iterator operator--(int) {
            iterator ret{*this};
            --*this;
            return ret;
        }

        node *_node;
        node *_end;
    };

    explicit sorted_tree(node *dummy, Compare compare = Compare())
            : _dummy(dummy), _comparator(compare) {}

    iterator insert(node *node) {
        if (_dummy->left == _dummy) {
            _dummy->left = node;
        } else {
            add(_dummy->left, node);
            _dummy->left = node;
        }
        ++_node_count;
        return iterator{node, _dummy};
    }

    iterator find(type const &value) const {
        node *curr = _dummy->left;
        while (curr != _dummy && (_comparator(curr->value, value) ||
                                  _comparator(value, curr->value))) {
            if (_comparator(value, curr->value)) {
                curr = curr->left;
            } else {
                curr = curr->right;
            }
        }

        return iterator{curr, _dummy};
    }

    iterator erase(node *n) {
        --_node_count;
        return iterator{remove(n), _dummy};
    }

    iterator lower_bound(T const &value) const {
        return {lower_bound(_dummy->left, value), _dummy};
    }

    iterator begin() const {
        node *curr = _dummy->left;
        while (curr->left != _dummy) {
            curr = curr->left;
        }
        return iterator{curr, _dummy};
    }

    iterator end() const { return iterator{_dummy, _dummy}; }

    Compare comparator() const { return _comparator; }

    size_t size() const { return _node_count; }

private:
    node *remove(node *remove_node) {
        splay(remove_node);
        iterator it{remove_node, _dummy};
        ++it;
        remove_node->left->parent = _dummy;
        remove_node->right->parent = _dummy;
        _dummy->left = merge(remove_node->left, remove_node->right);
        remove_node->left = _dummy;
        remove_node->right = _dummy;
        remove_node->parent = _dummy;
        return it._node;
    }

    void add(node *tree, node *insert_node) {
        insert_node->parent = tree->parent;
        tree->parent = _dummy;
        std::pair<node *, node *> trees = split(tree, insert_node->value);
        insert_node->left = trees.first;
        insert_node->right = trees.second;
        if (trees.first != _dummy) {
            trees.first->parent = insert_node;
        }
        if (trees.second != _dummy) {
            trees.second->parent = insert_node;
        }
    }

    node *merge(node *tree1, node *tree2) {
        if (tree1 == _dummy) {
            return tree2;
        }

        if (tree2 == _dummy) {
            return tree1;
        }

        while (tree1->right != _dummy) {
            tree1 = tree1->right;
        }
        splay(tree1);
        tree1->right = tree2;
        tree2->parent = tree1;
        return tree1;
    }

    node *lower_bound(node *tree, T const &value) const {
        node *curr = tree;
        while (curr != _dummy && (_comparator(curr->value, value) ||
                                  _comparator(value, curr->value))) {
            if (_comparator(value, curr->value)) {
                if (curr->left == _dummy) {
                    break;
                }
                curr = curr->left;
            } else {
                if (curr->right == _dummy) {
                    break;
                }
                curr = curr->right;
            }
        }

        iterator it{curr, _dummy};

        if (_comparator(curr->value, value)) {
            ++it;
        }

        return it._node;
    }

    std::pair<node *, node *> split(node *tree, T const &value) {
        node *curr = tree;
        while (curr != _dummy && (_comparator(curr->value, value) ||
                                  _comparator(value, curr->value))) {
            if (_comparator(value, curr->value)) {
                if (curr->left == _dummy) {
                    break;
                }
                curr = curr->left;
            } else {
                if (curr->right == _dummy) {
                    break;
                }
                curr = curr->right;
            }
        }

        splay(curr);

        if (_comparator(curr->value, value)) {
            node *temp = curr->right;
            if (temp != _dummy) {
                temp->parent = _dummy;
            }
            curr->right = _dummy;
            return {curr, temp};
        } else {
            node *temp = curr->left;
            if (temp != _dummy) {
                temp->parent = _dummy;
            }
            curr->left = _dummy;
            return {temp, curr};
        }
    }

    node *rotate_left(node *v) {
        node *p = v->parent;
        node *r = v->right;
        if (p->left == v) {
            p->left = r;
        } else {
            p->right = r;
        }

        node *tmp = r->left;
        r->left = v;
        v->right = tmp;
        v->parent = r;
        r->parent = p;
        if (v->right != _dummy) {
            v->right->parent = v;
        }

        return v;
    }

    node *rotate_right(node *v) {
        node *p = v->parent;
        node *l = v->left;
        if (p->left == v) {
            p->left = l;
        } else {
            p->right = l;
        }

        node *tmp = l->right;
        l->right = v;
        v->left = tmp;
        v->parent = l;
        l->parent = p;
        if (v->left != _dummy) {
            v->left->parent = v;
        }

        return v;
    }

    void splay(node *v) {
        while (v->parent != _dummy) {
            if (v == v->parent->left) {
                if (v->parent->parent == _dummy) {
                    rotate_right(v->parent);
                } else if (v->parent == v->parent->parent->left) {
                    rotate_right(v->parent->parent);
                    rotate_right(v->parent);
                } else {
                    rotate_right(v->parent);
                    rotate_left(v->parent);
                }
            } else {
                if (v->parent->parent == _dummy) {
                    rotate_left(v->parent);
                } else if (v->parent == v->parent->parent->right) {
                    rotate_left(v->parent->parent);
                    rotate_left(v->parent);
                } else {
                    rotate_left(v->parent);
                    rotate_right(v->parent);
                }
            }
        }
    }

    void normalise(node *n) {
        if (n != _dummy) {
            if (n->left != _dummy) {
                n->left->parent = n;
            }
            if (n->right != _dummy) {
                n->right->parent = n;
            }
        }
    }

    node *const _dummy;
    Compare _comparator;
    size_t _node_count{};
};

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
        typename CompareRight = std::less<Right>>
struct bimap {
    struct left_tag;
    struct right_tag;

    using left_tree = sorted_tree<Left, left_tag, CompareLeft>;
    using right_tree = sorted_tree<Right, right_tag, CompareRight>;

    using left_t = Left;
    using right_t = Right;

    struct node : left_tree::node, right_tree::node {
        template <class L, class R>
        node(L &&l_val, R &&r_val, node *const dummy)
                : left_tree::node{std::forward<L>(l_val), dummy, dummy, dummy},
                  right_tree::node{std::forward<R>(r_val), dummy, dummy, dummy} {}

        typename left_tree::node &left() {
            return static_cast<typename left_tree::node &>(*this);
        }
        typename right_tree::node &right() {
            return static_cast<typename right_tree::node &>(*this);
        }
    };

    using node_t = node;

    struct right_iterator;
    struct left_iterator;

    static left_iterator flip_to_left(right_iterator r);
    static right_iterator flip_to_right(left_iterator r);

    struct right_iterator {
        right_iterator(typename right_tree::iterator iterator)
                : _iterator(iterator) {}

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        right_t const &operator*() const { return *_iterator; }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        right_iterator &operator++() {
            ++_iterator;
            return *this;
        }

        right_iterator operator++(int) {
            right_iterator temp(*this);
            ++*this;
            return temp;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        right_iterator &operator--() {
            --_iterator;
            return *this;
        }

        right_iterator operator--(int) {
            right_iterator temp{*this};
            --*this;
            return temp;
        }

        [[nodiscard]] bool operator==(right_iterator l_it) const {
            return _iterator == l_it._iterator;
        }

        [[nodiscard]] bool operator!=(right_iterator l_it) const {
            return !(*this == l_it);
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        left_iterator flip() const { return flip_to_left(*this); }

        typename right_tree::node *node() const { return _iterator._node; }

        typename right_tree::iterator _iterator;
    };

    struct left_iterator {
        left_iterator(typename left_tree::iterator iterator)
                : _iterator(iterator) {}

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        left_t const &operator*() const { return *_iterator; }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        left_iterator &operator++() {
            ++_iterator;
            return *this;
        }
        left_iterator operator++(int) {
            left_iterator temp{*this};
            ++*this;
            return temp;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        left_iterator &operator--() {
            --_iterator;
            return *this;
        }

        left_iterator operator--(int) {
            left_iterator temp{*this};
            --*this;
            return temp;
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        right_iterator flip() const { return flip_to_right(*this); }

        typename left_tree::node *node() const { return _iterator._node; }

        [[nodiscard]] bool operator==(left_iterator l_it) const {
            return _iterator == l_it._iterator;
        }

        [[nodiscard]] bool operator!=(left_iterator l_it) const {
            return !(*this == l_it);
        }

        typename left_tree::iterator _iterator;
    };

    // Создает bimap не содержащий ни одной пары.
    bimap(CompareLeft compare_left = CompareLeft(),
          CompareRight compare_right = CompareRight())
            : _left_tree(_dummy, compare_left), _right_tree(_dummy, compare_right) {
        _dummy->left().parent = _dummy;
        _dummy->left().left = _dummy;
        _dummy->left().right = _dummy;

        _dummy->right().parent = _dummy;
        _dummy->right().left = _dummy;
        _dummy->right().right = _dummy;
    }

    // Конструкторы от других и присваивания
    bimap(bimap const &other)
            : bimap(other._left_tree.comparator(), other._right_tree.comparator()) {
        for (auto it = other.begin_left(); it != other.end_left(); ++it) {
            auto *n = static_cast<node_t *>(it.node());
            insert(n->left().value, n->right().value);
        }
    }

    bimap &operator=(bimap const &other) {
        _left_tree.comparator() = other._left_tree.comparator();
        _right_tree.comparator() = other._right_tree.comparator();
        for (auto it = other.begin_left(); it != other.end_left(); ++it) {
            auto *n = static_cast<node_t *>(it.node());
            insert(n->left().value, n->right().value);
        }
        return *this;
    }

    // Деструктор. Вызывается при удалении объектов bimap.
    // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
    // (включая итераторы ссылающиеся на элементы следующие за последними).
    ~bimap() { delete_tree(static_cast<node_t *>(_dummy->left().left)); }

    // Вставка пары (left, right), возвращает итератор на left.
    // Если такой left или такой right уже присутствуют в bimap, вставка не
    // производится и возвращается end_left().
    left_iterator insert(left_t const &left, right_t const &right) {
        if (find_left(left) != end_left() || find_right(right) != end_right()) {
            return end_left();
        }
        std::unique_ptr<node_t> node = create_node(left, right);
        _right_tree.insert(node.get());
        return _left_tree.insert(node.release());
    }

    left_iterator insert(left_t const &left, right_t &&right) {
        if (find_left(left) != end_left() || find_right(right) != end_right()) {
            return end_left();
        }
        std::unique_ptr<node_t> node = create_node(left, std::move(right));
        _right_tree.insert(node.get());
        return _left_tree.insert(node.release());
    }

    left_iterator insert(left_t &&left, right_t const &right) {
        if (find_left(left) != end_left() || find_right(right) != end_right()) {
            return end_left();
        }
        std::unique_ptr<node_t> node = create_node(std::move(left), right);
        _right_tree.insert(node.get());
        return _left_tree.insert(node.release());
    }

    left_iterator insert(left_t &&left, right_t &&right) {
        if (find_left(left) != end_left() || find_right(right) != end_right()) {
            return end_left();
        }
        std::unique_ptr<node_t> node =
                create_node(std::move(left), std::move(right));
        _right_tree.insert(node.get());
        return _left_tree.insert(node.release());
    }

    // Удаляет элемент и соответствующий ему парный.
    // erase невалидного итератора неопределен.
    // erase(end_left()) и erase(end_right()) неопределены.
    // Пусть it ссылается на некоторый элемент e.
    // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
    left_iterator erase_left(left_iterator it) {
        auto n = static_cast<node_t *>(it._iterator._node);
        auto ret_it = _left_tree.erase(n);
        _right_tree.erase(n);
        delete_tree(n);
        return ret_it;
    }
    // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
    // не делает ничего Возвращает была ли пара удалена
    bool erase_left(left_t const &left) {
        auto founded = _left_tree.find(left);
        _left_tree.erase(founded._node);
        _right_tree.erase(static_cast<node_t *>(founded._node));
        return founded._node != _dummy;
    }

    right_iterator erase_right(right_iterator it) {
        auto n = static_cast<node_t *>(it._iterator._node);
        auto ret_it = _right_tree.erase(n);
        _left_tree.erase(n);
        delete_tree(n);
        return ret_it;
    }

    bool erase_right(right_t const &right) {
        auto founded = _right_tree.find(right);
        if (founded._node != _dummy) {
            _right_tree.erase(founded._node);
            _left_tree.erase(static_cast<node_t *>(founded._node));
        }
        return founded._node != _dummy;
    }

    // erase от ренжа, удаляет [first, last), возвращает итератор на последний
    // элемент за удаленной последовательностью
    left_iterator erase_left(left_iterator first, left_iterator last) {
        while (first != last) {
            first = erase_left(first);
        }
        return first;
    }

    right_iterator erase_right(right_iterator first, right_iterator last) {
        while (first != last) {
            first = erase_right(first);
        }
        return first;
    }

    // Возвращает итератор по элементу. Если не найден - соответствующий end()
    left_iterator find_left(left_t const &left) const {
        return _left_tree.find(left);
    }

    right_iterator find_right(right_t const &right) const {
        return _right_tree.find(right);
    }

    // Возвращает противоположный элемент по элементу
    // Если элемента не существует -- бросает std::out_of_range
    right_t const &at_left(left_t const &key) const {
        auto l_node = find_left(key)._iterator._node;
        if (l_node == _dummy) {
            throw std::out_of_range{"at_left: not found"};
        }
        return static_cast<typename right_tree::node *>(
                static_cast<node_t *>(l_node))
                ->value;
    }

    left_t const &at_right(right_t const &key) const {
        auto r_node = find_right(key)._iterator._node;
        if (r_node == _dummy) {
            throw std::out_of_range{"at_right: not found"};
        }
        return static_cast<typename left_tree::node *>(
                static_cast<node_t *>(r_node))
                ->value;
    }

    // Возвращает противоположный элемент по элементу
    // Если элемента не существует, добавляет его в bimap и на противоположную
    // сторону кладет дефолтный элемент, ссылку на который и возвращает
    // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
    // соответствующий ему элемент на запрашиваемый (смотри тесты)
    right_t const &at_left_or_default(left_t const &key) {
        auto l_node = find_left(key)._iterator._node;
        if (l_node == _dummy) {
            auto r_node = find_right({})._iterator._node;
            if (r_node == _dummy) {
                l_node = insert(key, {})._iterator._node;
            } else {
                l_node = static_cast<node_t *>(r_node);
                l_node->value = key;
            }
        }
        return static_cast<typename right_tree::node *>(
                static_cast<node_t *>(l_node))
                ->value;
    }

    left_t const &at_right_or_default(right_t const &key) {
        auto r_node = find_right(key)._iterator._node;
        if (r_node == _dummy) {
            auto l_node = find_left({})._iterator._node;
            if (l_node == _dummy) {
                r_node = static_cast<node_t *>(insert({}, key)._iterator._node);
            } else {
                r_node = static_cast<node_t *>(l_node);
                r_node->value = key;
            }
        }
        return static_cast<typename left_tree::node *>(
                static_cast<node_t *>(r_node))
                ->value;
    }

    // lower и upper bound'ы по каждой стороне
    // Возвращают итераторы на соответствующие элементы
    // Смотри std::lower_bound, std::upper_bound.
    left_iterator lower_bound_left(const left_t &left) const {
        return _left_tree.lower_bound(left);
    }

    left_iterator upper_bound_left(const left_t &left) const {
        auto it = lower_bound_left(left);
        if (!_left_tree.comparator()(*it, left) &&
            !_left_tree.comparator()(left, *it)) {
            ++it;
        }
        return it;
    }

    right_iterator lower_bound_right(const right_t &left) const {
        return _right_tree.lower_bound(left);
    }
    right_iterator upper_bound_right(const right_t &left) const {
        auto it = lower_bound_right(left);
        if (!_right_tree.comparator()(*it, left) &&
            !_right_tree.comparator()(left, *it)) {
            ++it;
        }
        return it;
    }

    // Возващает итератор на минимальный по порядку left.
    left_iterator begin_left() const { return _left_tree.begin(); }
    // Возващает итератор на следующий за последним по порядку left.
    left_iterator end_left() const { return _left_tree.end(); }

    // Возващает итератор на минимальный по порядку right.
    right_iterator begin_right() const { return _right_tree.begin(); }
    // Возващает итератор на следующий за последним по порядку right.
    right_iterator end_right() const { return _right_tree.end(); }

    // Проверка на пустоту
    bool empty() const { return begin_left() == end_left(); }

    // Возвращает размер бимапы (кол-во пар)
    std::size_t size() const { return _left_tree.size(); }

    // операторы сравнения
    friend bool operator==(bimap const &a, bimap const &b) {
        if (a.size() != b.size()) {
            return false;
        }
        auto lit = b.begin_left();
        auto lit1 = a.begin_left();
        for (; lit1 != a.end_left() && lit != b.end_left(); ++lit1, ++lit) {
            if (*lit != *lit1)
                return false;
        }

        auto rit = b.begin_right();
        auto rit1 = a.begin_right();
        for (; rit1 != a.end_right() && rit != b.end_right(); ++rit1, ++rit) {
            if (*rit != *rit1)
                return false;
        }

        return true;
    }

    friend bool operator!=(bimap const &a, bimap const &b) { return !(a == b); }

private:
    template <class L, class R>
    std::unique_ptr<node_t> create_node(L &&l, R &&r) {
        return std::make_unique<node_t>(std::forward<L>(l), std::forward<R>(r),
                                        _dummy);
    }

    void delete_tree(node_t *n) {
        if (n == _dummy) {
            return;
        }

        delete_tree(static_cast<node_t *>(n->left().left));
        delete_tree(static_cast<node_t *>(n->left().right));
        delete n;
    }

    alignas(node_t) char fake_arr[sizeof(node_t)]{};
    node_t *_dummy = (node_t *)fake_arr;
    left_tree _left_tree;
    right_tree _right_tree;
};

template <typename Left, typename Right, typename CompareLeft,
        typename CompareRight>
typename bimap<Left, Right, CompareLeft, CompareRight>::left_iterator
bimap<Left, Right, CompareLeft, CompareRight>::flip_to_left(right_iterator r) {
    return bimap::left_iterator(
            typename left_tree::iterator(static_cast<node_t *>(r.node()),
                                         static_cast<node_t *>(r._iterator._end)));
}
template <typename Left, typename Right, typename CompareLeft,
        typename CompareRight>
typename bimap<Left, Right, CompareLeft, CompareRight>::right_iterator
bimap<Left, Right, CompareLeft, CompareRight>::flip_to_right(left_iterator r) {
    return bimap::right_iterator(
            typename right_tree::iterator(static_cast<node_t *>(r.node()),
                                          static_cast<node_t *>(r._iterator._end)));
}
 