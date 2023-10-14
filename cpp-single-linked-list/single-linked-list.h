#include <vector>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <algorithm>
#include <iostream>

template <typename Type>
class SingleLinkedList 
{
    // Узел списка
    struct Node 
    {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) 
            {}
        Type value;
        Node* next_node = nullptr;
    };

public:
    // Шаблон класса «Базовый Итератор».
    // ValueType — совпадает с Type (для Iterator) либо с const Type (для ConstIterator)
    template <typename ValueType>
    class BasicIterator 
    {
        // Класс списка объявляется дружественным, чтобы из методов списка
        // был доступ к приватной области итератора
        friend class SingleLinkedList;

        // Конвертирующий конструктор итератора из указателя на узел списка
        explicit BasicIterator(Node* node) 
        {
            node_ = node;
        }

    public:
        // Объявленные ниже типы сообщают стандартной библиотеке о свойствах этого итератора

        // Категория итератора — forward iterator
        // (итератор, который поддерживает операции инкремента и многократное разыменование)
        using iterator_category = std::forward_iterator_tag;
        // Тип элементов, по которым перемещается итератор
        using value_type = Type;
        // Тип, используемый для хранения смещения между итераторами
        using difference_type = std::ptrdiff_t;
        // Тип указателя на итерируемое значение
        using pointer = ValueType*;
        // Тип ссылки на итерируемое значение
        using reference = ValueType&;

        BasicIterator() = default;

        // Конвертирующий конструктор/конструктор копирования
        // При ValueType, совпадающем с Type, играет роль копирующего конструктора
        // При ValueType, совпадающем с const Type, играет роль конвертирующего конструктора
        BasicIterator(const BasicIterator<Type>& other) noexcept 
        {
            node_ = other.node_; 
        }

        // Чтобы компилятор не выдавал предупреждение об отсутствии оператора = при наличии
        // пользовательского конструктора копирования, явно объявим оператор = и
        // попросим компилятор сгенерировать его за нас
        BasicIterator& operator=(const BasicIterator& rhs) = default;

        // Оператор сравнения итераторов (в роли второго аргумента выступает константный итератор)
        // Два итератора равны, если они ссылаются на один и тот же элемент списка либо на end()
        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept 
        {
            return this->node_ == rhs.node_;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept 
        {
            return !(this->node_ == rhs.node_);
        }

        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept 
        {
            return this->node_ == rhs.node_;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept 
        {
            return !(this->node_ == rhs.node_);
        }

        // Оператор прединкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает ссылку на самого себя
        // Инкремент итератора, не указывающего на существующий элемент списка, приводит к неопределённому поведению
        BasicIterator& operator++() noexcept 
        {
            assert(this->node_ != nullptr); 
            *this = BasicIterator(node_->next_node);

            return *this;
        }

        // Оператор постинкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает прежнее значение итератора
        // Инкремент итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        BasicIterator operator++(int) noexcept 
        {
            BasicIterator old_node(node_); // Сохраняем прежнее значение объекта для последующего возврата
            ++(*this); // используем логику префиксной формы инкремента
            return old_node;
        }

        // Операция разыменования. Возвращает ссылку на текущий элемент
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] reference operator*() const noexcept 
        {
            assert(node_ != nullptr );
            return node_->value;
        }

        // Операция доступа к члену класса. Возвращает указатель на текущий элемент списка
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] pointer operator->() const noexcept 
        {
            assert(node_ != nullptr );
            return &(node_->value);
        }

    private:
        Node* node_ = nullptr;
    };

    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    [[nodiscard]] Iterator begin() noexcept 
    {
        // Благодаря дружбе SingleLinkedList имеет доступ к приватному конструктору своего итератора
        return Iterator{head_.next_node};
    }

    [[nodiscard]] Iterator end() noexcept 
    {
        // Благодаря дружбе SingleLinkedList имеет доступ к приватному конструктору своего итератора
        return Iterator();
    }

    // Константные версии begin/end для обхода списка без возможности модификации его элементов
    [[nodiscard]] ConstIterator begin() const noexcept 
    {
        return ConstIterator{cbegin()};
    }
    [[nodiscard]] ConstIterator end() const noexcept 
    {
        return ConstIterator(cend());  
    }

    // Методы для удобного получения константных итераторов у неконстантного контейнера
    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return ConstIterator{head_.next_node};
    }

    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return ConstIterator();  
    }

    SingleLinkedList()
    {
        size_ = 0;
    }

    SingleLinkedList(std::initializer_list<Type> values) 
    {
        this->init_range(values.begin(), values.end());
    }       

    SingleLinkedList(const SingleLinkedList& other) 
    {
        this->init_range(other.begin(), other.end());
    }

    SingleLinkedList& operator=(const SingleLinkedList& rhs) 
    {
        if(this != &rhs)
        {
            SingleLinkedList tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    // Обменивает содержимое списков за время O(1)
    void swap(SingleLinkedList& other) noexcept 
    {
        std::swap(this->head_.next_node, other.head_.next_node);
        std::swap(this->size_, other.size_);
    }

    ~SingleLinkedList()
    {
        Clear();
    }

    [[nodiscard]] size_t GetSize() const noexcept 
    {
        return size_;
    }

    [[nodiscard]] bool IsEmpty() const noexcept 
    {
        return  !(size_ > 0);
    }

    void PushFront(const Type& value) 
    {
        head_.next_node = new Node(value, head_.next_node);
        ++size_;
    }

    void Clear() noexcept 
    {
        Node* tmp_node;
        for ( int i = 0; i < size_; ++i)
        {
            tmp_node = head_.next_node;
            if ( tmp_node != nullptr )
            {
                head_.next_node = head_.next_node->next_node;
                delete tmp_node;
            }
        }
        size_ = 0;
    }

    // Возвращает итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator before_begin() noexcept 
    {
        return Iterator(&(head_));
    }

    [[nodiscard]] ConstIterator cbefore_begin() const noexcept 
    {
        return ConstIterator( const_cast<Node*>(&head_) );
    }

    [[nodiscard]] ConstIterator before_begin() const noexcept 
    {
        return cbefore_begin();
    }

    /*
     * Вставляет элемент value после элемента, на который указывает pos.
     * Возвращает итератор на вставленный элемент
     * Если при создании элемента будет выброшено исключение, список останется в прежнем состоянии
     */
    Iterator InsertAfter(ConstIterator pos, const Type& value) 
    {
        assert(pos.node_ != nullptr);
        
        Node* new_node = new Node(value, pos.node_->next_node);
        pos.node_->next_node = new_node;
        size_++;
        return Iterator(new_node);
    }

    void PopFront() noexcept 
    {
        assert( this->size_ != 0 );

        Node * prev_node = this->head_.next_node->next_node;
        delete this->head_.next_node;
        this->head_.next_node = prev_node;
        this->size_--;
    }

    /*
     * Удаляет элемент, следующий за pos.
     * Возвращает итератор на элемент, следующий за удалённым
     */
    Iterator EraseAfter(ConstIterator pos) noexcept 
    {
        assert(pos.node_ != nullptr);
        assert(pos.node_->next_node != nullptr);
        assert(size_ > 0);

        Node* node = pos.node_->next_node;
        pos.node_->next_node = pos.node_->next_node->next_node;
        delete node;
        this->size_--;
        return (this->size_ != 0 ) ? Iterator(pos.node_->next_node) : end();
    }

private:

    // Инициализация списка из переданного диапазона
    template <typename Iterator>
    void init_range(Iterator range_begin, Iterator range_end)
    {
        assert(size_ == 0 && head_.next_node == nullptr);

        SingleLinkedList tmp;

        auto  it_tmp = tmp.cbefore_begin();
        for (auto it = range_begin; it != range_end; ++it) 
        {
            it_tmp = tmp.InsertAfter(it_tmp, *it);
        }

        this->swap(tmp);
    }

    Node head_;
    size_t size_ = 0;
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept 
{
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    if (lhs.GetSize() != rhs.GetSize()) 
        return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    return (lhs < rhs || lhs == rhs);
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    return !(lhs < rhs);
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) 
{
    return (!(lhs < rhs) || lhs == rhs);
} 