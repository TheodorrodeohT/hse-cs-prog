// Лирическое отступление: на переписывании контрольной работы
// надо было найти общие числа в двух отсортированных массивах.
// Почему-то все стали изобретать свой велосипед вместо того,
// чтобы решать эту задачу через алгоритмы стандартной библиотеки.
// Вот как можно было бы написать требуемую функцию:

#include <algorithm>
#include <vector>

std::vector<int> vec_intersection(const std::vector<int>& v1, const std::vector<int>& v2) {
    std::vector<int> result;
    std::set_intersection(
        v1.begin(), v1.end(),
        v2.begin(), v2.end(),
        //result.begin()  // вы понимаете, почему тут нельзя писать result.begin()?
        std::back_inserter(result)
    );
    return result;
}

// Мораль: старайтесь использовать готовые конструкции из стандартной библиотеки там, где это уместно.


// В прошлый раз мы писали класс "Квадратная матрица", который был реализован через статический массив.
// Размер матрицы задавался как шаблонный параметр и должен был быть известным в момент компиляции программы.
// Сегодня мы попробуем сделать класс, получающий размер матрицы в конструкторе.
// Это позволит узнавать требуемый размер матрицы уже в runtime, когда программа уже запущена.

// Ещё мы попробуем хранить элементы матрицы в низкоуровневом динамическом массиве.
// Мы увидим, с какими потенциальными проблемами мы можем столкнуться, и поймём,
// почему предпочтительнее использовать специальные классы-обёртки вроде std::array или std::vector.


template <typename T>
class Matrix {
private:
    //T Data[N][N];  // так было в прошлый раз, но теперь N заранее неизвестно
    //std::vector<std::vector<T>> Data;  // так делать предпочтительнее
    const size_t N; // будем отдельно хранить размер матрицы 
    T** Data; // указатель на динамический массив со строками матрицы (каждая из которых будет динамическим массивом элементов)

public:
    Matrix(size_t n, const T& lambda = T()): N(n) {  // инициализируем сразу константу n
        Data = new T * [N];  // выделяем память под N строк, каждая из которых - это указатель на массив элементов
        for (size_t i = 0; i != N; ++i) {
            Data[i] = new T [N];  // теперь выделяем память под каждую строку
            Data[i][i] = lambda;
        }
    } // Код такого конструктора на самом деле опасен. Об этом - ниже.

    ~Matrix() {  // так как мы выделяли память вручную, то должны сами её освободить, когда матрица "умрёт"
        for (size_t i = 0; i != N; ++i)
            delete [] Data[i];
        delete [] Data;
    }

    // Мы забыли тут кое-что важное. Обсудим ниже.

    size_t size() const {
        return N;
    }

    T& operator () (size_t i, size_t j) {  // получение элемента по индексам - неконстантная и константная версии
        return Data[i][j];
    }

    const T& operator () (size_t i, size_t j) const {
            return Data[i][j];
    }

    T * operator[] (size_t i) {  // получение строки матрицы
        return Data[i];
    }

    const T * operator[] (size_t i) const {
        return Data[i];
    }
};

// Для наших матриц пока не определены никакие арифметические операции.
// Давайте пока заметим, что вот с таким кодом уже будут проблемы:

int main() {
    Matrix<int> A(5);
    Matrix<int> B = A;
} // внезапно тут происходит что-то странное


// Дело в том, что мы не определили для матрицы конструктор копирования.
// Да-да, в выражении Matrix<int> B = A вызывается именно конструктор, а не оператор присваивания.
// Значок = не должен вводить вас в заблуждение: тут создаётся новый объект B, а не изменяется уже существующий.

// Так вот, компилятор напишет конструктор копирования автоматически за нас. Этот конструктор будет просто копировать все поля.
// Этот автоматически написанный конструктор выглядел бы вот так, если бы мы его писали сами:

template <typename T>
Matrix<T>::Matrix<T>(const Matrix<T>& other)
    : N(other.N)
    , Data(other.Data)
{
}

// Он просто скопирует указатель Data. В результате указатели в A и в B будут ссылаться на одну и ту же память.
// Во-первых, это приведёт к логическим проблемам: изменяя одну из матриц, мы будем видеть изменения и в другой.
// А во-вторых, серьёзные проблемы возникнут тогда, когда начнут вызываться деструкторы.
// Сначала деструктор B освободит занимаемую память. Потом то же самое попробует сделать деструктор A, и программа "упадёт".

// Аналогичные проблемы произойдут и при использовании оператора присваивания:

int main() {
    Matrix<int> A(5);
    Matrix<int> B(5);
    B = A;  // B теперь опять ссылается на ту же память, что и A
}  // при вызове деструктора A возникнет проблема!

// Как избежать таких проблем?
// Надо определиться со стратегией владения памятью нашими матрицами.
// Например, можно вообще запретить копирование и присваивание матриц.
// Или можно написать честные операторы, которые выполняли бы "глубокое" копирование и присваивание, выделяя новую память:

template <typename T>
Matrix<T>::Matrix(const Matrix<T>& other): N(other.N) {
    Data = new T * [N];
    for (size_t i = 0; i != N; ++i) {
        Data[i] = new T [N];
        //for (size_t j = 0; j != N; ++j)
        //    Data[i][j] = other.Data[i][j];
        // Вместо этих закомментированных строк можно написать так:
        std::copy(other.Data[i], other.Data[i] + N, Data[i]);
    }
}

template <typename T>
Matrix<T>::operator = (const Matrix<T>& other) {
    Matrix<T> copy(other);  // реализуем оператор присваивания через уже написанный конструктор копирования
    std::swap(N, copy.N);
    std::swap(Data, copy.Data);  // обмениваемся данными с локальной копией
}  // локальная переменная copy теперь владеет старой памятью и умирает вот здесь


// Ещё можно представить себе стратегию, когда по умолчанию память не копируется, но на неё подсчитываются ссылки.
// Память удаляется в деструкторе только тогда, когда ссылок на неё больше нет.
// А при попытке модификации матрицы происходит "глубокое" копирование.


// Вернёмся теперь к арифметическим операторам.
// Что должно произойти, если мы попытаемся сложить или умножить матрицы разного размера?
// (Напоминаю, что теперь размер матрицы хранится у нас в самой матрице, а не является частью типа.)

// Самое правильное, что тут можно сделать, - сгенерировать исключение с помощью оператора throw.
// С исключением должен быть связан объект, несущий информацию о произошедшей ошибке.
// Он может быть любой природы (хоть int). Иногда рекомендуется наследовать его от стандартного типа (например, std::exception).

// Аналогичная ситуация возникает, когда мы пытаемся построить обратную матрицу к вырожденной матрице.

class DifferentSizeException {  // Тип исключения для операций над матрицами разного размера
public:
    size_t N1, N2;  // те самые размеры матриц, которые не совпали
};


class ZeroDeterminantException {  // Тип исключения для попытки обращения вырожденной матрицы
};


template <typename T>
Matrix<T>& operator += (
        Matrix<T>& A,
        const Matrix<T>& B
) {
    if (A.size() != B.size())
        throw DifferentSizeException{A.size(), B.size()};  // создаём и инициализируем объект исключения на лету
    for (size_t i = 0; i != A.size(); ++i)
        for (size_t j = 0; j != A.size(); ++j)
            A(i, j) += B(i, j);
    return A;
}

template <typename T>
Matrix<T> inverse(const Matrix<T>& A) {
    Matrix<T> result(A.size());
    // Код обращения матрицы писать тут не будем - реализуйте метод Гаусса сами
    if (/* определитель == 0 */)
        throw ZeroDeterminantException();  // понимаете, зачем тут скобки?
    return result;
}


// Если произошло исключение, то работа функции прерывается,
// все созданные к этому моменту локальные переменные автоматически уничтожаются (как это произошло бы при нормальном выходе из функции),
// и исключение "пробрасывыется" на уровень выше, туда, откуда наша функция была вызвана.
// И всё это повторяется до тех пор, пока не встретится обработчик try/catch.

int main() {
    try {
        Matrix<MyInteger> A(3);
        Matrix<MyInteger> B(17);
        A += B;  // вот тут произойдёт исключение типа DifferentSizeException!
        auto C = inverse(A);  // а сюда мы дойти не успеем
    } catch (ZeroDeterminantException) {  // обработчики исключений разных типов
        std::cout << "Zero det!\n";
    } catch (const DifferentSizeException& ex) {  // мы попадём сюда
        std::cout << "Different sizes: "  // напечатаем сообщение об ошибке
                  << ex.N1 << " and "
                  << ex.N2 << "!\n";
        throw;  // и, например, пробросим это исключение дальше, если мы не знаем, что делать с этой ошибкой
    } catch (...) {  // универсальный обработчик, обрабатывающий любые исключения; может идти только последним
        std::cout << "Unknown exception caught!\n";
    }
}

// Обработчики в блоках catch рассматриваются один за другим.
// При возникновении исключения в блоке try выполняется код первого обработчика с соответствующим типом исключения.
// Можно думать об обработчиках как о наборе перегруженных функций с одним аргументом - объектом исключения.

// Исключения можно генерировать и в конструкторах.
// Это самый правильный способ сообщить, что объект не может быть создан.
// В этом случае автоматически удаляются все уже проинициализированные поля объекта и локальные переменные конструктора.

// А вот в деструкторах генерировать исключения категорически не рекомендуется,
// так как любой деструктор может быть вызван при обработке исключения.


// Посмотрим теперь ещё раз на код, который мы написали в конструкторах:
/*1*/    Data = new T * [N];
/*2*/    for (size_t i = 0; i != N; ++i) {
/*3*/        Data[i] = new T [N];
/*4*/        // ...
/*5*/    }
// Этот код опасен. Он может привести к утечке памяти.
// Например, в строке 3 на какой-нибудь итерации цикла может произойти исключение по двум причинам:
// либо в операторе new из-за нехватки динамической памяти, либо в конструкторе неизвестного нам класса T.
// В этом случае объект матрицы не будет считаться созданным, и его деструктор, конечно, не вызовется.
// Память, выделенная в строке 1 и в строке 3 на предыдущих итерациях, безвозвратно утечёт.

// В этом можно убедиться, подставив вместо T искусственный класс MyInteger, генерирующий исключение, скажем,
// при создании своего четвёртого экземпляра:

class MyInteger {
    static int counter;  // статическая переменная - она является общей для всех объектов класса
public:
    MyInteger(int n = 0) {
        ++counter;  // увеличиваем число созданных экземпляров
        if (counter > 3)
            throw 42;
    }
};

int MyInteger::counter = 0;  // инициализация статической переменной


// Как обезопасить наш конструктор?
// Можно было бы сделать так:
    Data = new T * [N];
    for (size_t i = 0; i != N; ++i) {
        try {
            Data[i] = new T [N];
        } catch (...) {
            for (size_t j = 0; j != i; ++j)  // удаляем уже созданные строки
                 delete [] Data[j];
            delete [] Data;  // удаляем массив самих строк
            throw;
        }
        // ...
    }

// Код от таких проверок становится неуклюжим.
// Не забудьте, что аналогичные проверки нужны и в конструкторе копирования.
// А теперь задумайтесь, что и вот тут может произойти исключение:
template <typename T>
Matrix<T>::Matrix(const Matrix<T>& other): N(other.N) {
    Data = new T * [N];
    for (size_t i = 0; i != N; ++i) {
        Data[i] = new T [N];
        std::copy(other.Data[i], other.Data[i] + N, Data[i]);  // копирование объектов неизвестного типа T может закончиться неудачей!
    }
}

// Как же сделать так, чтобы код не был перегружен в каждом месте неуклюжими проверками на исключения
// для предотвращения утечек памяти?
// Общая рекомендация - использовать идиому RAII (Resource aquitization is initialization).
// Переводится это как "выделение ресурса должно быть инициализацией".
// Если бы наша переменная Data была бы не голым указателем, а объектом специального класса, "оборачивающего" этот указатель,
// то для неё бы вызывался бы деструктор всякий раз, когда происходило бы исключение. 
// И в этом деструкторе мы бы безболезненно удаляли бы динамическую память.
// Выделение динамической памяти соответствовало бы конструированию (инициализации) объекта этого класса - отсюда и название идиомы.

// В роли такого класса-обёртки вполне может выступать std::vector или std::array:
template <typename T>
class Matrix {
private:
    std::vector<std::vector<T>> Data;
public:
    Matrix(size_t n, const T& lambda = T()): Data(n) {
        for (size_t i = 0; i != n; ++i)
             Data[i].resize(n);
             Data[i][i] = lambda;
    }  // что бы тут не произошло, для объекта Data будет вызван деструктор, а в нем вся выделенная память корректно будет освобождена

    // Конструктор копирования и оператор присваивания можно вообще не писать - векторы прекрасно делают это сами.

    size_t size() const {
        return Data.size();
    }

    // ...
};

// Мораль: всегда предпочитайте контейнеры стандартной библиотеки низкоуровневым массивам.

