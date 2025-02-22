// source.cs
using System;

namespace SampleApp {
    class Program {
        static void Main(string[] args) {
            int age = 30;
            string name = "Alice";
            Console.WriteLine("Hello, " + name);
        }

        static void Greet(string name) {
            Console.WriteLine("Greetings, " + name);
        }
    }
}
