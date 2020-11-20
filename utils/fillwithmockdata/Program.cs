using Npgsql;
using System;
using System.Linq;
using System.Text.RegularExpressions;

namespace fillwithmockdata
{
    class Program
    {
        static void Main(string[] args)
        {
            var cs = "Host=localhost;Username=postgres;Password=changeme;Database=urls_remap";

            using var con = new NpgsqlConnection(cs);
            con.Open();

            var sql = "SELECT version()";
            using var cmd = new NpgsqlCommand(sql, con);
            var version = cmd.ExecuteScalar().ToString();
            Console.WriteLine($"PostgreSQL version: {version}");
            DropTableData(con);
            InsertMockData(con);
        }

        private static void DropTableData(NpgsqlConnection con)
        {
            var sql = "DELETE FROM mapping;";
            using var cmd = new NpgsqlCommand(sql, con);
            Console.WriteLine($"Deleted: {cmd.ExecuteNonQuery()} rows");
        }

        static string CodeURLWithNumber(int number)
        {
            var guid = new Guid(number, 0, 0, new byte[8]);
            return new string(Regex.Replace(Convert.ToBase64String(guid.ToByteArray()), "[/+=]", "").Take(6).ToArray());
        }

        private static void InsertMockData(NpgsqlConnection con)
        {
            var random = new Random(0);
            foreach (var i in Enumerable.Range(0, 32000000))
            {
                try
                {
                    var sql = @"INSERT INTO mapping(orig_url, new_url, created_on, expired_on, sms_uuid)"
                            + "VALUES(@orig_url, @new_url, @created_on, @expired_on, @sms_uuid)";

                    var cmd = new NpgsqlCommand(sql, con);
                    cmd.Parameters.AddWithValue("orig_url", $"http://url_number_{i}");
                    cmd.Parameters.AddWithValue("new_url", $"http://localhost:12000/{CodeURLWithNumber(i)}");

                    cmd.Parameters.AddWithValue("created_on", DateTime.Now);
                    cmd.Parameters.AddWithValue("expired_on", DateTime.Now.Add(TimeSpan.FromDays(365)));

                    cmd.Parameters.AddWithValue("sms_uuid", "");
                    cmd.Prepare();
                    cmd.ExecuteNonQuery();
                }
                catch { }
            }
        }
    }
}
